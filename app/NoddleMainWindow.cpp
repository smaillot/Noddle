#include "NoddleMainWindow.hpp"
#include "NoddleConnectionPainter.hpp"
#include "PreviewPanel.hpp"
#include "TimelineView.hpp"
#include "engine/PipelineExecutor.hpp"
#include "widgets/PipelineProfiler.hpp"

#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionIdUtils.hpp>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QClipboard>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QSettings>
#include <QStatusBar>
#include <QUndoCommand>
#include <QUndoStack>

#include "nodes/ImageSourceModel.hpp"
#include "nodes/CsvSourceModel.hpp"
#include "nodes/PointCloudSourceModel.hpp"
#include "nodes/CameraSourceModel.hpp"
#include "nodes/ImageDisplayModel.hpp"
#include "nodes/PythonPluginModel.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/ColorConvertModel.hpp"
#include "nodes/opencv/ResizeModel.hpp"
#include "nodes/opencv/GaussianBlurModel.hpp"
#include "nodes/opencv/ThresholdModel.hpp"
#include "nodes/opencv/CannyModel.hpp"
#include "nodes/opencv/MorphologyModel.hpp"
#endif

using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::NodeDelegateModelRegistry;

namespace {

// Serialize selected nodes with internal-only connections (both endpoints selected).
QJsonObject serializeSelectionInternalOnly(DataFlowGraphicsScene *scene,
                                           DataFlowGraphModel *model)
{
    using QtNodes::NodeGraphicsObject;

    std::unordered_set<QtNodes::NodeId> selectedIds;
    for (auto *item : scene->selectedItems()) {
        if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item))
            selectedIds.insert(ngo->nodeId());
    }

    QJsonArray nodesJson;
    for (auto id : selectedIds)
        nodesJson.append(model->saveNode(id));

    QJsonArray connsJson;
    std::set<std::tuple<int, int, int, int>> seen;
    for (auto id : selectedIds) {
        for (auto const &cid : model->allConnectionIds(id)) {
            if (selectedIds.count(cid.outNodeId) && selectedIds.count(cid.inNodeId)) {
                auto key = std::make_tuple(
                    static_cast<int>(cid.outNodeId), cid.outPortIndex,
                    static_cast<int>(cid.inNodeId), cid.inPortIndex);
                if (seen.insert(key).second)
                    connsJson.append(QtNodes::toJson(cid));
            }
        }
    }

    QJsonObject result;
    result["nodes"] = nodesJson;
    result["connections"] = connsJson;
    return result;
}

// Copy selected nodes (internal connections only) to clipboard.
void copySelectedToClipboard(DataFlowGraphicsScene *scene, DataFlowGraphModel *model)
{
    QJsonObject json = serializeSelectionInternalOnly(scene, model);
    if (json["nodes"].toArray().isEmpty())
        return;

    auto *mimeData = new QMimeData();
    QByteArray data = QJsonDocument(json).toJson();
    mimeData->setData("application/qt-nodes-graph", data);
    mimeData->setText(data);
    QApplication::clipboard()->setMimeData(mimeData);
}

// Duplicate selected nodes without external connections.
// Internal connections (both endpoints selected) are preserved.
class DuplicateCommand : public QUndoCommand
{
public:
    DuplicateCommand(DataFlowGraphicsScene *scene, DataFlowGraphModel *model,
                     QPointF const &pastePos)
        : m_scene(scene), m_model(model)
    {
        using QtNodes::NodeGraphicsObject;

        // Collect selected node IDs
        std::unordered_set<QtNodes::NodeId> selectedIds;
        for (auto *item : scene->selectedItems()) {
            if (auto *ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item))
                selectedIds.insert(ngo->nodeId());
        }

        if (selectedIds.empty()) {
            setObsolete(true);
            return;
        }

        // Access newNodeId() through the public base class interface
        auto &graphModel = scene->graphModel();

        // Compute average position of selected nodes
        QPointF avgPos(0, 0);
        for (auto id : selectedIds) {
            QJsonObject nj = model->saveNode(id);
            auto pos = nj["position"].toObject();
            avgPos += QPointF(pos["x"].toDouble(), pos["y"].toDouble());
        }
        avgPos /= static_cast<double>(selectedIds.size());
        QPointF offset = pastePos - avgPos;

        // Build new node JSONs with remapped IDs and offset positions
        std::unordered_map<QtNodes::NodeId, QtNodes::NodeId> idMap;
        for (auto oldId : selectedIds) {
            QJsonObject nodeJson = model->saveNode(oldId);
            QtNodes::NodeId newId = graphModel.newNodeId();
            idMap[oldId] = newId;

            nodeJson["id"] = static_cast<qint64>(newId);

            QJsonObject posJson = nodeJson["position"].toObject();
            posJson["x"] = posJson["x"].toDouble() + offset.x();
            posJson["y"] = posJson["y"].toDouble() + offset.y();
            nodeJson["position"] = posJson;

            m_nodesJson.append(nodeJson);
        }

        // Collect internal connections only (both endpoints selected)
        std::set<std::tuple<int, int, int, int>> seen;
        for (auto oldId : selectedIds) {
            for (auto const &cid : model->allConnectionIds(oldId)) {
                if (selectedIds.count(cid.outNodeId) && selectedIds.count(cid.inNodeId)) {
                    auto key = std::make_tuple(
                        static_cast<int>(cid.outNodeId), cid.outPortIndex,
                        static_cast<int>(cid.inNodeId), cid.inPortIndex);
                    if (seen.insert(key).second) {
                        QtNodes::ConnectionId newCid{idMap[cid.outNodeId], cid.outPortIndex,
                                                     idMap[cid.inNodeId], cid.inPortIndex};
                        m_connsJson.append(QtNodes::toJson(newCid));
                    }
                }
            }
        }

        setText("Duplicate");
    }

    void undo() override
    {
        // Delete connections first
        for (auto const &cv : m_connsJson) {
            m_model->deleteConnection(QtNodes::fromJson(cv.toObject()));
        }
        // Delete nodes
        for (auto const &nv : m_nodesJson) {
            m_model->deleteNode(nv.toObject()["id"].toInt());
        }
    }

    void redo() override
    {
        m_scene->clearSelection();
        // Load nodes
        for (auto const &nv : m_nodesJson) {
            QJsonObject nj = nv.toObject();
            m_model->loadNode(nj);
            auto *ngo = m_scene->nodeGraphicsObject(nj["id"].toInt());
            if (ngo) {
                ngo->setZValue(1.0);
                ngo->setSelected(true);
            }
        }
        // Load internal connections
        for (auto const &cv : m_connsJson) {
            m_model->addConnection(QtNodes::fromJson(cv.toObject()));
        }
    }

private:
    DataFlowGraphicsScene *m_scene;
    DataFlowGraphModel *m_model;
    QJsonArray m_nodesJson;
    QJsonArray m_connsJson;
};

} // namespace

NoddleMainWindow::NoddleMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_registry = createRegistry();
    m_graphModel = new DataFlowGraphModel(m_registry);
    m_graphModel->setParent(this);
    m_scene = new DataFlowGraphicsScene(*m_graphModel, this);
    m_scene->setConnectionPainter(std::make_unique<NoddleConnectionPainter>());
    m_graphicsView = new GraphicsView(m_scene);

    setCentralWidget(m_graphicsView);
    resize(1200, 800);

    // Preview panel (right dock)
    m_previewPanel = new PreviewPanel(*m_graphModel, this);
    addDockWidget(Qt::RightDockWidgetArea, m_previewPanel);

    // Timeline panel (bottom dock)
    m_timelineView = new TimelineView(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineView);

    // When a dock floats, promote it to a top-level window so it can be
    // moved to another screen. Re-docking restores default dock flags.
    auto enableDetach = [this](QDockWidget *dock, Qt::DockWidgetArea area) {
        QObject::connect(dock, &QDockWidget::topLevelChanged, dock, [this, dock, area](bool floating) {
            if (floating) {
                dock->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint
                                     | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
                                     | Qt::WindowMinMaxButtonsHint);
                dock->show();
            } else {
                // Restore default dock widget flags when re-docked
                dock->setWindowFlags(Qt::Widget);
                addDockWidget(area, dock);
                dock->show();
            }
        });
    };
    enableDetach(m_previewPanel, Qt::RightDockWidgetArea);
    enableDetach(m_timelineView, Qt::BottomDockWidgetArea);

    // Status bar pipeline time (refresh at 5 Hz)
    m_pipelineTimeLabel = new QLabel("Pipeline: — ms");
    m_pipelineTimeLabel->setStyleSheet("color: #aaa; margin-right: 8px;");
    statusBar()->addPermanentWidget(m_pipelineTimeLabel);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(200);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        double ms = PipelineProfiler::instance().totalPipelineMs(
            PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::Sec1);
        if (ms < 0.01) {
            m_pipelineTimeLabel->setText(QStringLiteral("Pipeline: \u2014"));
        } else {
            // Show FPS only if a camera source is actively producing frames
            double camFps = PipelineProfiler::instance().fps(QStringLiteral("Camera Source"));
            if (camFps > 0.1)
                m_pipelineTimeLabel->setText(
                    QString("Pipeline: %1 ms (%2 fps)").arg(ms, 0, 'f', 2).arg(camFps, 0, 'f', 1));
            else
                m_pipelineTimeLabel->setText(
                    QString("Pipeline: %1 ms").arg(ms, 0, 'f', 2));
        }
    });
    m_statusTimer->start();

    connect(m_scene, &DataFlowGraphicsScene::nodeSelected,
            m_previewPanel, &PreviewPanel::onNodeSelected);

    connect(m_scene, &DataFlowGraphicsScene::modified,
            this, &NoddleMainWindow::onSceneModified);

    m_executor = new noddle::PipelineExecutor(*m_graphModel, this);

    setupMenus();

    connect(m_executor, &noddle::PipelineExecutor::nodeOutputReady,
            m_previewPanel, [this](QtNodes::NodeId nodeId, QtNodes::PortIndex) {
        m_previewPanel->refreshForNode(nodeId);
    });

    setupStatusBar();
    updateWindowTitle();
}

void NoddleMainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void NoddleMainWindow::setupMenus()
{
    // --- File menu ---
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *newAct = fileMenu->addAction(tr("&New"), this, &NoddleMainWindow::onNewFile);
    newAct->setShortcut(QKeySequence::New);

    auto *openAct = fileMenu->addAction(tr("&Open…"), this, &NoddleMainWindow::onOpenFile);
    openAct->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    auto *saveAct = fileMenu->addAction(tr("&Save"), this, &NoddleMainWindow::onSaveFile);
    saveAct->setShortcut(QKeySequence::Save);

    auto *saveAsAct = fileMenu->addAction(tr("Save &As…"), this, &NoddleMainWindow::onSaveFileAs);
    saveAsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    fileMenu->addSeparator();

    m_recentMenu = fileMenu->addMenu(tr("Recent &Pipelines"));
    updateRecentFilesMenu();

    fileMenu->addSeparator();

    auto *quitAct = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // --- Edit menu ---
    // Note: Ctrl+Z / Ctrl+Shift+Z shortcuts are already registered
    // by QtNodes GraphicsView. Menu items work via mouse click.
    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_scene->undoStack().createUndoAction(this, tr("&Undo")));
    editMenu->addAction(m_scene->undoStack().createRedoAction(this, tr("&Redo")));

    editMenu->addSeparator();

    auto *cutAct = editMenu->addAction(tr("Cu&t"));
    connect(cutAct, &QAction::triggered, this, [this]() {
        copySelectedToClipboard(m_scene, m_graphModel);
        m_graphicsView->onDeleteSelectedObjects();
    });

    editMenu->addAction(tr("&Copy"), this, [this]() {
        copySelectedToClipboard(m_scene, m_graphModel);
    });

    editMenu->addAction(tr("&Paste"), m_graphicsView,
                        &QtNodes::GraphicsView::onPasteObjects);

    editMenu->addSeparator();

    editMenu->addAction(tr("&Duplicate"), this, [this]() {
        QPointF pastePos = m_graphicsView->mapToScene(
            m_graphicsView->mapFromGlobal(QCursor::pos()));
        QRect vr = m_graphicsView->rect();
        QPoint local = m_graphicsView->mapFromGlobal(QCursor::pos());
        if (!vr.contains(local))
            pastePos = m_graphicsView->mapToScene(vr.center());

        m_scene->undoStack().push(
            new DuplicateCommand(m_scene, m_graphModel, pastePos));
    });

    // Replace the built-in Ctrl+D (which duplicates WITH external connections)
    for (auto *act : m_graphicsView->actions()) {
        if (act->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_D)) {
            m_graphicsView->removeAction(act);
            delete act;
            break;
        }
    }
    auto *dupShortcut = new QAction(this);
    dupShortcut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    dupShortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(dupShortcut, &QAction::triggered, this, [this]() {
        QPointF pastePos = m_graphicsView->mapToScene(
            m_graphicsView->mapFromGlobal(QCursor::pos()));
        QRect vr = m_graphicsView->rect();
        QPoint local = m_graphicsView->mapFromGlobal(QCursor::pos());
        if (!vr.contains(local))
            pastePos = m_graphicsView->mapToScene(vr.center());

        m_scene->undoStack().push(
            new DuplicateCommand(m_scene, m_graphModel, pastePos));
    });
    m_graphicsView->addAction(dupShortcut);

    // Replace the built-in Ctrl+C (which copies WITH external connections)
    for (auto *act : m_graphicsView->actions()) {
        if (act->shortcut() == QKeySequence(QKeySequence::Copy)) {
            m_graphicsView->removeAction(act);
            delete act;
            break;
        }
    }
    auto *copyShortcut = new QAction(this);
    copyShortcut->setShortcut(QKeySequence(QKeySequence::Copy));
    copyShortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QAction::triggered, this, [this]() {
        copySelectedToClipboard(m_scene, m_graphModel);
    });
    m_graphicsView->addAction(copyShortcut);

    // Replace the built-in Ctrl+X (which copies WITH external connections before deleting)
    for (auto *act : m_graphicsView->actions()) {
        if (act->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_X)) {
            m_graphicsView->removeAction(act);
            delete act;
            break;
        }
    }
    auto *cutShortcut = new QAction(this);
    cutShortcut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_X));
    cutShortcut->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(cutShortcut, &QAction::triggered, this, [this]() {
        copySelectedToClipboard(m_scene, m_graphModel);
        m_graphicsView->onDeleteSelectedObjects();
    });
    m_graphicsView->addAction(cutShortcut);

    editMenu->addAction(tr("D&elete"), m_graphicsView,
                        &QtNodes::GraphicsView::onDeleteSelectedObjects);

    // --- Pipeline menu ---
    auto *pipelineMenu = menuBar()->addMenu(tr("&Pipeline"));

    auto *runAct = pipelineMenu->addAction(tr("&Run"));
    runAct->setShortcut(QKeySequence(Qt::Key_F5));

    auto *stopAct = pipelineMenu->addAction(tr("&Stop"));
    stopAct->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
    stopAct->setEnabled(false);

    auto *pauseAct = pipelineMenu->addAction(tr("&Pause"));
    pauseAct->setShortcut(QKeySequence(Qt::Key_F6));
    pauseAct->setCheckable(true);
    pauseAct->setEnabled(false);

    connect(runAct, &QAction::triggered, this, [this, runAct, stopAct, pauseAct]() {
        m_executor->start();
        runAct->setEnabled(false);
        stopAct->setEnabled(true);
        pauseAct->setEnabled(true);
    });

    connect(stopAct, &QAction::triggered, this, [this, runAct, stopAct, pauseAct]() {
        m_executor->stop();
        runAct->setEnabled(true);
        stopAct->setEnabled(false);
        pauseAct->setEnabled(false);
        pauseAct->setChecked(false);
    });

    connect(pauseAct, &QAction::toggled, this, [this](bool checked) {
        m_executor->setPaused(checked);
    });

    connect(m_executor, &noddle::PipelineExecutor::stopped, this, [runAct, stopAct, pauseAct]() {
        runAct->setEnabled(true);
        stopAct->setEnabled(false);
        pauseAct->setEnabled(false);
        pauseAct->setChecked(false);
    });

    // --- View menu ---
    auto *viewMenu = menuBar()->addMenu(tr("&View"));

    auto *togglePreview = m_previewPanel->toggleViewAction();
    togglePreview->setText(tr("Preview Panel"));
    viewMenu->addAction(togglePreview);

    auto *toggleTimeline = m_timelineView->toggleViewAction();
    toggleTimeline->setText(tr("Pipeline Timeline"));
    viewMenu->addAction(toggleTimeline);

    viewMenu->addSeparator();

    auto *redockAll = viewMenu->addAction(tr("Re-dock All Panels"), this, [this]() {
        auto redock = [](QDockWidget *dock) {
            if (dock->isFloating()) {
                dock->setFloating(false);
            }
        };
        redock(m_previewPanel);
        redock(m_timelineView);
    });
    redockAll->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));

    // Add benchmark and profiler actions to the Pipeline menu
    pipelineMenu->addSeparator();
    pipelineMenu->addAction(tr("&Benchmark…"), this, &NoddleMainWindow::onBenchmarkPipeline);
    pipelineMenu->addAction(tr("&Clear Profiler"), this, []() {
        PipelineProfiler::instance().clear();
    });
}

void NoddleMainWindow::setupStatusBar()
{
    int nodeCount = static_cast<int>(m_registry->registeredModelsCategoryAssociation().size());
    statusBar()->showMessage(
        tr("Ready — %1 node types registered").arg(nodeCount));
}

std::shared_ptr<NodeDelegateModelRegistry> NoddleMainWindow::createRegistry()
{
    auto registry = std::make_shared<NodeDelegateModelRegistry>();

    // Source nodes
    registry->registerModel<ImageSourceModel>("Sources");
    registry->registerModel<CsvSourceModel>("Sources");
    registry->registerModel<PointCloudSourceModel>("Sources");
    registry->registerModel<CameraSourceModel>("Sources");

    // Display nodes
    registry->registerModel<ImageDisplayModel>("Display");

    // Custom nodes
    registry->registerModel<PythonPluginModel>("Custom");

#ifdef NODDLE_WITH_OPENCV
    // OpenCV processing nodes
    registry->registerModel<ColorConvertModel>("OpenCV");
    registry->registerModel<ResizeModel>("OpenCV");
    registry->registerModel<GaussianBlurModel>("OpenCV");
    registry->registerModel<ThresholdModel>("OpenCV");
    registry->registerModel<CannyModel>("OpenCV");
    registry->registerModel<MorphologyModel>("OpenCV");
#endif

    return registry;
}

void NoddleMainWindow::updateWindowTitle()
{
    QString title = "Noddle — Visual Pipeline Editor";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fi(m_currentFilePath);
        title = QString("%1 — Noddle").arg(fi.fileName());
    }
    if (m_modified) {
        title.prepend("● ");
    }
    setWindowTitle(title);
}

bool NoddleMainWindow::maybeSave()
{
    if (!m_modified)
        return true;

    auto ret = QMessageBox::warning(
        this, tr("Unsaved Changes"),
        tr("The pipeline has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        onSaveFile();
        return !m_modified; // still modified if save was cancelled
    }
    return ret == QMessageBox::Discard;
}

void NoddleMainWindow::onNewFile()
{
    if (!maybeSave())
        return;

    m_scene->clearScene();
    m_scene->undoStack().clear();

    m_currentFilePath.clear();
    m_modified = false;
    updateWindowTitle();
    setupStatusBar();
}

void NoddleMainWindow::onOpenFile()
{
    if (!maybeSave())
        return;

    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open Pipeline"), pipelinesDir(),
        tr("Noddle Pipeline (*.ndl *.noddle);;All Files (*)"));

    if (filePath.isEmpty())
        return;

    loadFromFile(filePath);
}

void NoddleMainWindow::onSaveFile()
{
    if (m_currentFilePath.isEmpty()) {
        onSaveFileAs();
        return;
    }
    saveToFile(m_currentFilePath);
}

void NoddleMainWindow::onSaveFileAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Save Pipeline"), pipelinesDir(),
        tr("Noddle Pipeline (*.ndl);;All Files (*)"));

    if (filePath.isEmpty())
        return;

    saveToFile(filePath);
}

void NoddleMainWindow::onSceneModified()
{
    m_modified = true;
    updateWindowTitle();
}

void NoddleMainWindow::loadFromFile(QString const &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot open file:\n%1").arg(filePath));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Invalid pipeline file:\n%1").arg(filePath));
        return;
    }

    // Clear existing graph using scene's official method
    m_scene->clearScene();

    m_graphModel->load(doc.object());

    // Reset undo stack so undo doesn't replay deletions
    m_scene->undoStack().clear();

    m_currentFilePath = filePath;
    m_modified = false;
    updateWindowTitle();
    addRecentFile(filePath);

    int nodeCount = static_cast<int>(m_graphModel->allNodeIds().size());
    statusBar()->showMessage(
        tr("Loaded — %1 nodes").arg(nodeCount), 3000);

    // Restore the view transform if saved, otherwise fit to scene
    QJsonObject root = doc.object();
    if (root.contains("viewTransform")) {
        QJsonObject vt = root["viewTransform"].toObject();
        double scale = vt["scale"].toDouble(1.0);
        double cx = vt["centerX"].toDouble(0.0);
        double cy = vt["centerY"].toDouble(0.0);
        m_graphicsView->resetTransform();
        m_graphicsView->scale(scale, scale);
        m_graphicsView->centerOn(cx, cy);
    } else {
        m_graphicsView->centerScene();
    }
}

void NoddleMainWindow::saveToFile(QString const &filePath)
{
    QString actualPath = filePath;
    if (QFileInfo(actualPath).suffix().isEmpty()) {
        actualPath += ".ndl";
    }

    QFile file(actualPath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot write file:\n%1").arg(actualPath));
        return;
    }

    QJsonObject json = m_graphModel->save();

    // Workaround: QtNodes may not sync dragged QGraphicsObject positions
    // back to the model data. Read positions from the scene objects and
    // patch the JSON array so saved files reflect actual node placement.
    {
        QJsonArray nodesArray = json["nodes"].toArray();
        for (int i = 0; i < nodesArray.size(); ++i) {
            QJsonObject nodeJson = nodesArray[i].toObject();
            QtNodes::NodeId nid = static_cast<QtNodes::NodeId>(nodeJson["id"].toInt());
            if (auto *ngo = m_scene->nodeGraphicsObject(nid)) {
                QPointF pos = ngo->pos();
                QJsonObject posJson;
                posJson["x"] = pos.x();
                posJson["y"] = pos.y();
                nodeJson["position"] = posJson;
                nodesArray[i] = nodeJson;
            }
        }
        json["nodes"] = nodesArray;
    }

    // Save view transform (scale + center)
    QTransform t = m_graphicsView->transform();
    QPointF viewCenter = m_graphicsView->mapToScene(
        m_graphicsView->viewport()->rect().center());
    QJsonObject vt;
    vt["scale"] = t.m11();
    vt["centerX"] = viewCenter.x();
    vt["centerY"] = viewCenter.y();
    json["viewTransform"] = vt;

    QJsonDocument doc(json);
    file.write(doc.toJson());

    m_currentFilePath = actualPath;
    m_modified = false;
    updateWindowTitle();
    addRecentFile(actualPath);
    statusBar()->showMessage(tr("Saved"), 3000);
}

QString NoddleMainWindow::pipelinesDir() const
{
    if (!m_currentFilePath.isEmpty())
        return QFileInfo(m_currentFilePath).absolutePath();

    // Default to pipelines/ in the project source tree (set at compile time)
    QString dir = QStringLiteral(NODDLE_SOURCE_DIR "/pipelines");
    QDir().mkpath(dir);
    return dir;
}

void NoddleMainWindow::addRecentFile(QString const &filePath)
{
    QSettings settings("Noddle", "Noddle");
    QStringList files = settings.value("recentFiles").toStringList();
    files.removeAll(filePath);
    files.prepend(filePath);
    while (files.size() > kMaxRecentFiles)
        files.removeLast();
    settings.setValue("recentFiles", files);
    updateRecentFilesMenu();
}

void NoddleMainWindow::updateRecentFilesMenu()
{
    if (!m_recentMenu)
        return;

    m_recentMenu->clear();

    QSettings settings("Noddle", "Noddle");
    QStringList files = settings.value("recentFiles").toStringList();

    if (files.isEmpty()) {
        m_recentMenu->addAction(tr("(empty)"))->setEnabled(false);
        return;
    }

    for (auto const &path : files) {
        QFileInfo fi(path);
        auto *act = m_recentMenu->addAction(fi.fileName());
        act->setData(path);
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, &NoddleMainWindow::onOpenRecent);
    }

    m_recentMenu->addSeparator();
    m_recentMenu->addAction(tr("Clear"), this, [this]() {
        QSettings s("Noddle", "Noddle");
        s.remove("recentFiles");
        updateRecentFilesMenu();
    });
}

void NoddleMainWindow::onOpenRecent()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;

    QString filePath = act->data().toString();
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("File not found:\n%1").arg(filePath));
        return;
    }

    if (!maybeSave())
        return;

    loadFromFile(filePath);
}

void NoddleMainWindow::onBenchmarkPipeline()
{
    using QtNodes::NodeDelegateModel;
    using QtNodes::NodeId;
    using QtNodes::PortType;
    using QtNodes::PortIndex;
    using QtNodes::PortRole;

    // Collect source nodes with data
    auto allIds = m_graphModel->allNodeIds();
    struct SourcePort {
        NodeId nodeId;
        PortIndex port;
    };
    QVector<SourcePort> sources;

    for (auto nodeId : allIds) {
        auto *model = m_graphModel->delegateModel<NodeDelegateModel>(nodeId);
        if (!model || model->nPorts(PortType::In) > 0)
            continue;
        for (unsigned int p = 0; p < model->nPorts(PortType::Out); ++p) {
            auto data = model->outData(p);
            if (data)
                sources.append({nodeId, p});
        }
    }

    if (sources.isEmpty()) {
        QMessageBox::information(this, tr("Benchmark"),
                                 tr("No source nodes with data found.\n"
                                    "Load an image or connect a source first."));
        return;
    }

    bool ok = false;
    int iterations = QInputDialog::getInt(
        this, tr("Benchmark Pipeline"),
        tr("Number of iterations:"), 100, 1, 10000, 1, &ok);
    if (!ok)
        return;

    PipelineProfiler::instance().clear();

    QProgressDialog progress(tr("Benchmarking…"), tr("Cancel"), 0, iterations, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    for (int i = 0; i < iterations; ++i) {
        if (progress.wasCanceled())
            break;

        for (auto const &sp : sources) {
            auto conns = m_graphModel->connections(sp.nodeId, PortType::Out, sp.port);
            QVariant portDataVar = m_graphModel->portData(
                sp.nodeId, PortType::Out, sp.port, PortRole::Data);
            for (auto const &cn : conns) {
                m_graphModel->setPortData(cn.inNodeId, PortType::In,
                                          cn.inPortIndex, portDataVar, PortRole::Data);
            }
        }

        progress.setValue(i + 1);
    }

    progress.close();

    double totalMs = PipelineProfiler::instance().totalPipelineMs(
        PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::AllTime);

    QMessageBox::information(this, tr("Benchmark Results"),
                             tr("Iterations: %1\nAvg pipeline time: %2 ms")
                                 .arg(iterations)
                                 .arg(totalMs, 0, 'f', 2));
}
