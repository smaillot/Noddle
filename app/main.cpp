#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

#include "nodes/ImageSourceModel.hpp"
#include "nodes/CsvSourceModel.hpp"
#include "nodes/PointCloudSourceModel.hpp"
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

static std::shared_ptr<NodeDelegateModelRegistry> createRegistry()
{
    auto registry = std::make_shared<NodeDelegateModelRegistry>();

    // Source nodes
    registry->registerModel<ImageSourceModel>("Sources");
    registry->registerModel<CsvSourceModel>("Sources");
    registry->registerModel<PointCloudSourceModel>("Sources");

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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Noddle");

    auto registry = createRegistry();
    DataFlowGraphModel graph_model(registry);
    DataFlowGraphicsScene scene(graph_model);
    GraphicsView view(&scene);

    QMainWindow window;
    window.setWindowTitle("Noddle — Visual Pipeline Editor");
    window.setCentralWidget(&view);
    window.resize(1200, 800);

    int nodeCount = static_cast<int>(registry->registeredModelsCategoryAssociation().size());
    window.statusBar()->showMessage(
        QString("Ready — %1 node types registered").arg(nodeCount));

    window.show();

    return app.exec();
}
