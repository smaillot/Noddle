#pragma once

#include <QMainWindow>
#include <memory>

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

class PreviewPanel;

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

private:
    void setupMenus();
    void setupStatusBar();
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> createRegistry();
    void updateWindowTitle();
    bool maybeSave();
    void loadFromFile(QString const &filePath);
    void saveToFile(QString const &filePath);

    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> m_registry;
    QtNodes::DataFlowGraphModel *m_graphModel;
    QtNodes::DataFlowGraphicsScene *m_scene;
    QtNodes::GraphicsView *m_graphicsView;
    PreviewPanel *m_previewPanel;

    QString m_currentFilePath;
    bool m_modified = false;
};
