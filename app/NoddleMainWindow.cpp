#include "NoddleMainWindow.hpp"
#include "PreviewPanel.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QUndoStack>

#include "nodes/ImageSourceModel.hpp"
#include "nodes/CsvSourceModel.hpp"
#include "nodes/PointCloudSourceModel.hpp"
#include "nodes/CameraSourceModel.hpp"
#include "nodes/ImageDisplayModel.hpp"

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

NoddleMainWindow::NoddleMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_registry = createRegistry();
    m_graphModel = new DataFlowGraphModel(m_registry);
    m_graphModel->setParent(this);
    m_scene = new DataFlowGraphicsScene(*m_graphModel, this);
    m_graphicsView = new GraphicsView(m_scene);

    setCentralWidget(m_graphicsView);
    resize(1200, 800);

    // Preview panel (right dock)
    m_previewPanel = new PreviewPanel(*m_graphModel, this);
    addDockWidget(Qt::RightDockWidgetArea, m_previewPanel);

    connect(m_scene, &DataFlowGraphicsScene::nodeSelected,
            m_previewPanel, &PreviewPanel::onNodeSelected);

    connect(m_scene, &DataFlowGraphicsScene::modified,
            this, &NoddleMainWindow::onSceneModified);

    setupMenus();
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

    auto *quitAct = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // --- Edit menu ---
    // Note: Ctrl+Z / Ctrl+Shift+Z shortcuts are already registered
    // by QtNodes GraphicsView. Menu items work via mouse click.
    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_scene->undoStack().createUndoAction(this, tr("&Undo")));
    editMenu->addAction(m_scene->undoStack().createRedoAction(this, tr("&Redo")));

    // --- View menu ---
    auto *viewMenu = menuBar()->addMenu(tr("&View"));

    auto *togglePreview = m_previewPanel->toggleViewAction();
    togglePreview->setText(tr("Preview Panel"));
    viewMenu->addAction(togglePreview);
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

    // Clear the graph by reloading an empty JSON object
    QJsonObject empty;
    empty["nodes"] = QJsonArray();
    empty["connections"] = QJsonArray();
    m_graphModel->load(empty);

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
        this, tr("Open Pipeline"), QString(),
        tr("Noddle Pipeline (*.noddle);;All Files (*)"));

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
        this, tr("Save Pipeline"), QString(),
        tr("Noddle Pipeline (*.noddle);;All Files (*)"));

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

    // Clear existing graph before loading
    auto nodeIds = m_graphModel->allNodeIds();
    for (auto id : nodeIds) {
        m_graphModel->deleteNode(id);
    }

    m_graphModel->load(doc.object());
    m_currentFilePath = filePath;
    m_modified = false;
    updateWindowTitle();

    int nodeCount = static_cast<int>(m_graphModel->allNodeIds().size());
    statusBar()->showMessage(
        tr("Loaded — %1 nodes").arg(nodeCount), 3000);
}

void NoddleMainWindow::saveToFile(QString const &filePath)
{
    QString actualPath = filePath;
    if (!actualPath.contains('.')) {
        actualPath += ".noddle";
    }

    QFile file(actualPath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Cannot write file:\n%1").arg(actualPath));
        return;
    }

    QJsonObject json = m_graphModel->save();
    QJsonDocument doc(json);
    file.write(doc.toJson());

    m_currentFilePath = actualPath;
    m_modified = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Saved"), 3000);
}
