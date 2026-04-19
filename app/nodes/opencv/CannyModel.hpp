#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#endif

class CannyModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("Canny"); }
    QString caption() const override { return QStringLiteral("Canny Edge"); }

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
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_lowSpin = new QSpinBox();
            m_lowSpin->setRange(0, 500);
            m_lowSpin->setValue(50);

            m_highSpin = new QSpinBox();
            m_highSpin->setRange(0, 500);
            m_highSpin->setValue(150);

            layout->addWidget(new QLabel("Low"));
            layout->addWidget(m_lowSpin);
            layout->addWidget(new QLabel("High"));
            layout->addWidget(m_highSpin);

            connect(m_lowSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
            connect(m_highSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
        }
        return m_widget;
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
        cv::Mat gray;
        if (src.channels() == 3)
            cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
        else
            gray = src;

        cv::Mat dst;
        cv::Canny(gray, dst, m_lowSpin->value(), m_highSpin->value());
        m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
        m_output = m_input;
#endif
        Q_EMIT dataUpdated(0);
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_lowSpin = nullptr;
    QSpinBox *m_highSpin = nullptr;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
