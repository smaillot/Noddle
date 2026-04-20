#pragma once

#include <QImage>
#include <QtNodes/NodeData>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

enum class ColorSpace : uint8_t {
    RGB,
    BGR,
    Grayscale,
    HSV,
    HLS,
    Lab,
    YCrCb,
    XYZ,
};

constexpr int channelCount(ColorSpace cs) noexcept {
    return cs == ColorSpace::Grayscale ? 1 : 3;
}

constexpr char const* colorSpaceName(ColorSpace cs) noexcept {
    switch (cs) {
        case ColorSpace::RGB:       return "RGB";
        case ColorSpace::BGR:       return "BGR";
        case ColorSpace::Grayscale: return "Gray";
        case ColorSpace::HSV:       return "HSV";
        case ColorSpace::HLS:       return "HLS";
        case ColorSpace::Lab:       return "Lab";
        case ColorSpace::YCrCb:     return "YCrCb";
        case ColorSpace::XYZ:       return "XYZ";
    }
    return "?";
}

constexpr int colorSpaceCount() noexcept { return 8; }

class ImageData : public NodeData
{
public:
    explicit ImageData(QImage image, ColorSpace cs = ColorSpace::RGB)
        : m_image(std::move(image)), m_colorSpace(cs) {}

    NodeDataType type() const override { return {"image", "Image"}; }

    QImage const &image() const { return m_image; }
    ColorSpace colorSpace() const { return m_colorSpace; }
    int channels() const { return channelCount(m_colorSpace); }

private:
    QImage m_image;
    ColorSpace m_colorSpace = ColorSpace::RGB;
};
