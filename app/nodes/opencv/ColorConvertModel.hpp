#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QComboBox>

#include "data/ImageData.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#endif

class ColorConvertModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("ColorConvert"); }
    QString caption() const override { return QStringLiteral("Color Convert"); }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return (portType == QtNodes::PortType::In || portType == QtNodes::PortType::Out) ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return ImageData(QImage()).type();
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex) override
    {
        m_input = std::dynamic_pointer_cast<ImageData>(data);
        process();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_output;
    }

    QWidget *embeddedWidget() override
    {
        if (!m_combo) {
            m_combo = new QComboBox();
            m_combo->addItems({"Grayscale", "HSV", "Lab"});
            connect(m_combo, &QComboBox::currentIndexChanged, this, [this]() { process(); });
        }
        return m_combo;
    }

private:
    void process()
    {
        if (!m_input || m_input->image().isNull()) {
            m_output.reset();
            Q_EMIT dataUpdated(0);
            return;
        }

#ifdef NODDLE_WITH_OPENCV
        cv::Mat src = qImageToMat(m_input->image());
        cv::Mat dst;
        int code = cv::COLOR_RGB2GRAY;
        if (m_combo) {
            switch (m_combo->currentIndex()) {
            case 0: code = cv::COLOR_RGB2GRAY; break;
            case 1: code = cv::COLOR_RGB2HSV; break;
            case 2: code = cv::COLOR_RGB2Lab; break;
            }
        }
        cv::cvtColor(src, dst, code);
        m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
        m_output = m_input;
#endif
        Q_EMIT dataUpdated(0);
    }

    QComboBox *m_combo = nullptr;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
