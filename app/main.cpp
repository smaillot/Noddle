#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::NodeDelegateModelRegistry;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Noddle");

    // Empty registry — nodes will be registered in Phase 1
    auto registry = std::make_shared<NodeDelegateModelRegistry>();

    DataFlowGraphModel graph_model(registry);
    DataFlowGraphicsScene scene(graph_model);
    GraphicsView view(&scene);

    QMainWindow window;
    window.setWindowTitle("Noddle — Visual Pipeline Editor");
    window.setCentralWidget(&view);
    window.resize(1200, 800);
    window.statusBar()->showMessage("Ready — no nodes registered yet");
    window.show();

    return app.exec();
}
