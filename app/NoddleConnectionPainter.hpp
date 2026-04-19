#pragma once

#include <QtNodes/internal/AbstractConnectionPainter.hpp>
#include <QtNodes/internal/DefaultConnectionPainter.hpp>

class NoddleConnectionPainter : public QtNodes::AbstractConnectionPainter
{
public:
    void paint(QPainter *painter,
               QtNodes::ConnectionGraphicsObject const &cgo) const override;

    QPainterPath getPainterStroke(
        QtNodes::ConnectionGraphicsObject const &cgo) const override;

private:
    QPointF midPoint(QtNodes::ConnectionGraphicsObject const &cgo) const;
    QString compactLabel(QtNodes::ConnectionGraphicsObject const &cgo) const;
    QString detailedLabel(QtNodes::ConnectionGraphicsObject const &cgo) const;

    QtNodes::DefaultConnectionPainter m_defaultPainter;
};
