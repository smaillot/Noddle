#include "PreviewPanel.hpp"
#include "data/ImageData.hpp"

#include <QPixmap>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModel>

using QtNodes::DataFlowGraphModel;
using QtNodes::NodeId;
using QtNodes::NodeRole;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

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
    m_previewLabel->setStyleSheet("background-color: #2b2b2b; color: #888; border: 1px solid #444;");
    layout->addWidget(m_previewLabel, 1);

    layout->addStretch();
    setWidget(container);
}

QImage PreviewPanel::extractPreviewImage(NodeId nodeId)
{
    // Try output ports first, then input ports
    for (auto portType : {PortType::Out, PortType::In}) {
        unsigned int nPorts = m_graphModel.nodeData(nodeId,
            portType == PortType::Out ? NodeRole::OutPortCount : NodeRole::InPortCount)
            .toUInt();

        for (unsigned int i = 0; i < nPorts; ++i) {
            auto *model = m_graphModel.delegateModel<NodeDelegateModel>(nodeId);
            if (!model)
                continue;

            std::shared_ptr<QtNodes::NodeData> data;
            if (portType == PortType::Out) {
                data = model->outData(static_cast<PortIndex>(i));
            }
            if (!data)
                continue;

            auto *imgData = dynamic_cast<ImageData *>(data.get());
            if (imgData && !imgData->image().isNull())
                return imgData->image();
        }
    }
    return {};
}

void PreviewPanel::onNodeSelected(NodeId nodeId)
{
    QString caption = m_graphModel.nodeData(nodeId, NodeRole::Caption).toString();
    m_nodeNameLabel->setText(QString("Node: %1").arg(caption));

    QImage image = extractPreviewImage(nodeId);
    if (!image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        m_previewLabel->setPixmap(
            pixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_previewLabel->setText("No preview available");
    }
}
