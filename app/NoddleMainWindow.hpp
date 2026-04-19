#pragma once

#include <QMainWindow>
#include <QStringList>
#include <memory>

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

class QLabel;
class QMenu;
class QTimer;
class PreviewPanel;
class TimelineView;

namespace noddle { class PipelineExecutor; }

class NoddleMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit NoddleMainWindow(QWidget *parent = nullptr);
    ~NoddleMainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveFileAs();
    void onSceneModified();
    void onOpenRecent();

private:
    void setupMenus();
    void setupStatusBar();
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> createRegistry();
    void updateWindowTitle();
    bool maybeSave();
    void loadFromFile(QString const &filePath);
    void saveToFile(QString const &filePath);
    QString pipelinesDir() const;
    void addRecentFile(QString const &filePath);
    void updateRecentFilesMenu();

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> m_registry;
    QtNodes::DataFlowGraphModel *m_graphModel;
    QtNodes::DataFlowGraphicsScene *m_scene;
    QtNodes::GraphicsView *m_graphicsView;
    PreviewPanel *m_previewPanel;
    TimelineView *m_timelineView;

    noddle::PipelineExecutor *m_executor = nullptr;

    QLabel *m_pipelineTimeLabel = nullptr;
    QTimer *m_statusTimer = nullptr;

    QString m_currentFilePath;
    bool m_modified = false;

    QMenu *m_recentMenu = nullptr;
    static constexpr int kMaxRecentFiles = 8;
};
