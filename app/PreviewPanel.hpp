#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>

#include <QtNodes/Definitions>

#include "widgets/FpsCounter.hpp"

class QTimer;

namespace QtNodes {
class DataFlowGraphModel;
}

class PreviewPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QtNodes::DataFlowGraphModel &graphModel,
                          QWidget *parent = nullptr);

public slots:
    void onNodeSelected(QtNodes::NodeId nodeId);

private slots:
    void onNodeDataUpdated(QtNodes::NodeId nodeId, QtNodes::PortType portType, QtNodes::PortIndex portIndex);
    void onUpdateFpsLabel();

private:
    void refreshPreview();
    QImage extractPreviewImage(QtNodes::NodeId nodeId);

    QtNodes::DataFlowGraphModel &m_graphModel;
    QtNodes::NodeId m_selectedNodeId = QtNodes::InvalidNodeId;

    QLabel *m_nodeNameLabel;
    QLabel *m_previewLabel;
    QLabel *m_fpsLabel;

    FpsCounter m_fpsCounter;
    QTimer *m_fpsTimer;
};
