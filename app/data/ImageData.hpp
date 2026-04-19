#pragma once

#include <QImage>
#include <QtNodes/NodeData>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class ImageData : public NodeData
{
public:
    explicit ImageData(QImage image)
        : m_image(std::move(image))
    {}

    NodeDataType type() const override
    {
        return {"image", "Image"};
    }

    QImage const &image() const { return m_image; }

private:
    QImage m_image;
};
