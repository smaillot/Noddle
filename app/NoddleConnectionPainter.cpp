#include "NoddleConnectionPainter.hpp"

#include <QtNodes/internal/AbstractGraphModel.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/ConnectionState.hpp>
#include <QtNodes/internal/Definitions.hpp>
#include <QtNodes/NodeData>

#include "data/ImageData.hpp"
#include "data/TensorData.hpp"
#include "data/TableData.hpp"
#include "data/PointCloudData.hpp"
#include "widgets/PipelineProfiler.hpp"

#include <QPainter>
#include <QFontMetrics>

using QtNodes::ConnectionGraphicsObject;
using QtNodes::ConnectionId;
using QtNodes::NodeData;
using QtNodes::PortRole;
using QtNodes::PortType;

void NoddleConnectionPainter::paint(QPainter *painter,
                                     ConnectionGraphicsObject const &cgo) const
{
    // Delegate line drawing to the default painter
    m_defaultPainter.paint(painter, cgo);

    // Only show labels on established connections
    if (cgo.connectionState().requiresPort())
        return;

    bool const hovered = cgo.connectionState().hovered();
    bool const selected = cgo.isSelected();

    QString text = (hovered || selected) ? detailedLabel(cgo) : compactLabel(cgo);
    if (text.isEmpty())
        return;

    QPointF mid = midPoint(cgo);

    painter->save();

    QFont font;
    font.setPixelSize(hovered || selected ? 11 : 9);
    painter->setFont(font);

    QFontMetrics fm(font);
    QRectF textRect = fm.boundingRect(text);
    // Add padding
    qreal hPad = 5.0;
    qreal vPad = 2.0;
    QRectF pillRect(mid.x() - textRect.width() / 2.0 - hPad,
                    mid.y() - textRect.height() / 2.0 - vPad,
                    textRect.width() + 2 * hPad,
                    textRect.height() + 2 * vPad);

    // Semi-transparent background pill
    QColor bg(30, 30, 30, hovered || selected ? 200 : 140);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawRoundedRect(pillRect, 4.0, 4.0);

    // Text
    QColor fg(220, 220, 220, hovered || selected ? 255 : 180);
    painter->setPen(fg);
    painter->drawText(pillRect, Qt::AlignCenter, text);

    painter->restore();
}

QPainterPath NoddleConnectionPainter::getPainterStroke(
    ConnectionGraphicsObject const &cgo) const
{
    return m_defaultPainter.getPainterStroke(cgo);
}

QPointF NoddleConnectionPainter::midPoint(
    ConnectionGraphicsObject const &cgo) const
{
    // Midpoint of cubic bezier at t=0.5:
    // B(0.5) = (P0 + 3*C1 + 3*C2 + P3) / 8
    QPointF const p0 = cgo.out();
    QPointF const p3 = cgo.in();
    auto [c1, c2] = cgo.pointsC1C2();

    return (p0 + 3.0 * c1 + 3.0 * c2 + p3) / 8.0;
}

QString NoddleConnectionPainter::compactLabel(
    ConnectionGraphicsObject const &cgo) const
{
    auto const &cId = cgo.connectionId();
    auto const &model = cgo.graphModel();

    auto dataTypeVar = model.portData(cId.outNodeId, PortType::Out,
                                      cId.outPortIndex, PortRole::DataType);
    if (!dataTypeVar.isValid())
        return {};

    auto dataType = dataTypeVar.value<QtNodes::NodeDataType>();

    // Show FPS if source node has it
    QString nodeCaption = model.nodeData(cId.outNodeId, QtNodes::NodeRole::Caption).toString();
    double fpsVal = PipelineProfiler::instance().fps(nodeCaption);
    if (fpsVal > 0.1)
        return QString("%1 %2fps").arg(dataType.name).arg(fpsVal, 0, 'f', 1);

    return dataType.name;
}

QString NoddleConnectionPainter::detailedLabel(
    ConnectionGraphicsObject const &cgo) const
{
    auto const &cId = cgo.connectionId();
    auto const &model = cgo.graphModel();

    // Get data type name
    auto dataTypeVar = model.portData(cId.outNodeId, PortType::Out,
                                      cId.outPortIndex, PortRole::DataType);
    if (!dataTypeVar.isValid())
        return {};

    auto dataType = dataTypeVar.value<QtNodes::NodeDataType>();
    QString label = dataType.name;

    // Get actual data for metadata
    auto dataVar = model.portData(cId.outNodeId, PortType::Out,
                                  cId.outPortIndex, PortRole::Data);
    if (!dataVar.isValid())
        return label;

    auto nodeData = dataVar.value<std::shared_ptr<QtNodes::NodeData>>();
    if (!nodeData)
        return label;

    // Extract type-specific metadata
    if (auto *img = dynamic_cast<ImageData const *>(nodeData.get())) {
        QSize s = img->image().size();
        label += QString(" %1\u00D7%2").arg(s.width()).arg(s.height());
    } else if (auto *tbl = dynamic_cast<TableData const *>(nodeData.get())) {
        label += QString(" %1\u00D7%2").arg(tbl->rowCount()).arg(tbl->colCount());
    } else if (auto *pc = dynamic_cast<PointCloudData const *>(nodeData.get())) {
        label += QString(" %1 pts").arg(pc->size());
    }

    // Add FPS if available
    QString nodeCaption = model.nodeData(cId.outNodeId, QtNodes::NodeRole::Caption).toString();
    double fpsVal = PipelineProfiler::instance().fps(nodeCaption);
    if (fpsVal > 0.1)
        label += QString(" @ %1fps").arg(fpsVal, 0, 'f', 1);

    return label;
}
