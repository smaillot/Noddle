#include "PreviewPanel.hpp"
#include "data/ImageData.hpp"

#include <QPixmap>
#include <QSizePolicy>
#include <QTimer>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModel>

using QtNodes::DataFlowGraphModel;
using QtNodes::NodeId;
using QtNodes::NodeRole;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::InvalidNodeId;

PreviewPanel::PreviewPanel(DataFlowGraphModel &graphModel, QWidget *parent)
    : QDockWidget("Preview", parent)
    , m_graphModel(graphModel)
{
    setMinimumWidth(300);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    m_nodeNameLabel = new QLabel("Node: \u2014", container);
    m_nodeNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_nodeNameLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    layout->addWidget(m_nodeNameLabel);

    m_previewLabel = new QLabel("Select a node to preview", container);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(280, 200);
    m_previewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_previewLabel->setStyleSheet("background-color: #2b2b2b; color: #888; border: 1px solid #444;");
    layout->addWidget(m_previewLabel, 1);

    m_fpsLabel = new QLabel("FPS: —", container);
    m_fpsLabel->setStyleSheet("color: #aaa; font-size: 10px; padding: 2px 4px;");
    layout->addWidget(m_fpsLabel);

    layout->addStretch();
    setWidget(container);

    // Auto-refresh: listen for data changes on any node
    connect(&m_graphModel, &DataFlowGraphModel::inPortDataWasSet,
            this, &PreviewPanel::onNodeDataUpdated);

    // FPS update timer
    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000);
    connect(m_fpsTimer, &QTimer::timeout, this, &PreviewPanel::onUpdateFpsLabel);
}

QImage PreviewPanel::extractPreviewImage(NodeId nodeId)
{
    auto *model = m_graphModel.delegateModel<NodeDelegateModel>(nodeId);
    if (!model)
        return {};

    // Try output ports first
    unsigned int outPorts = m_graphModel.nodeData(nodeId, NodeRole::OutPortCount).toUInt();
    for (unsigned int i = 0; i < outPorts; ++i) {
        auto data = model->outData(static_cast<PortIndex>(i));
        if (!data)
            continue;
        auto *imgData = dynamic_cast<ImageData *>(data.get());
        if (imgData && !imgData->image().isNull())
            return imgData->image();
    }

    return {};
}

void PreviewPanel::onNodeSelected(NodeId nodeId)
{
    // Disconnect previous source node's dataUpdated signal
    if (m_selectedNodeId != InvalidNodeId) {
        auto *prevModel = m_graphModel.delegateModel<NodeDelegateModel>(m_selectedNodeId);
        if (prevModel) {
            disconnect(prevModel, &NodeDelegateModel::dataUpdated,
                       this, nullptr);
        }
    }

    m_selectedNodeId = nodeId;

    // Connect to selected node's dataUpdated for source nodes (no input ports)
    auto *model = m_graphModel.delegateModel<NodeDelegateModel>(nodeId);
    if (model) {
        connect(model, &NodeDelegateModel::dataUpdated,
                this, [this](PortIndex) { refreshPreview(); });
    }

    QString caption = m_graphModel.nodeData(nodeId, NodeRole::Caption).toString();
    m_nodeNameLabel->setText(QString("Node: %1").arg(caption));

    m_fpsTimer->start();
    refreshPreview();
}

void PreviewPanel::onNodeDataUpdated(NodeId nodeId, PortType, PortIndex)
{
    // Auto-refresh if the updated node is the one currently selected
    if (nodeId == m_selectedNodeId)
        refreshPreview();
}

void PreviewPanel::refreshPreview()
{
    if (m_selectedNodeId == InvalidNodeId)
        return;

    QImage image = extractPreviewImage(m_selectedNodeId);
    if (!image.isNull()) {
        m_fpsCounter.tick();
        QPixmap pixmap = QPixmap::fromImage(image);
        m_previewLabel->setPixmap(
            pixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_previewLabel->setText("No preview available");
    }
}

void PreviewPanel::onUpdateFpsLabel()
{
    m_fpsLabel->setText(QString("FPS: %1").arg(m_fpsCounter.fps(), 0, 'f', 1));
}
