#include "PreviewPanel.hpp"

#include <QPixmap>
#include <QWidget>

#include <QtNodes/DataFlowGraphModel>

using QtNodes::DataFlowGraphModel;
using QtNodes::NodeId;
using QtNodes::NodeRole;

PreviewPanel::PreviewPanel(DataFlowGraphModel &graphModel, QWidget *parent)
    : QDockWidget("Preview", parent)
    , m_graphModel(graphModel)
{
    setMinimumWidth(300);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);

    m_nodeNameLabel = new QLabel("Node: —", container);
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

void PreviewPanel::onNodeSelected(NodeId nodeId)
{
    // Get the node caption
    QString caption = m_graphModel.nodeData(nodeId, NodeRole::Caption).toString();
    m_nodeNameLabel->setText(QString("Node: %1").arg(caption));

    // Try to grab the node's embedded widget for preview
    QVariant widgetVariant = m_graphModel.nodeData(nodeId, NodeRole::Widget);
    auto *widget = widgetVariant.value<QWidget *>();

    if (widget && widget->isVisible()) {
        QPixmap pixmap = widget->grab();
        m_previewLabel->setPixmap(
            pixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_previewLabel->setText("No preview available");
    }
}
