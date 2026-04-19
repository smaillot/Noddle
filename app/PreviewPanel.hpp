#pragma once

#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>

#include <QtNodes/Definitions>

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

private:
    QImage extractPreviewImage(QtNodes::NodeId nodeId);

    QtNodes::DataFlowGraphModel &m_graphModel;
    QLabel *m_nodeNameLabel;
    QLabel *m_previewLabel;
};
