#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#endif

class ResizeModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("Resize"); }
    QString caption() const override { return QStringLiteral("Resize"); }

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
            auto *layout = new QHBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_widthSpin = new QSpinBox();
            m_widthSpin->setRange(1, 8192);
            m_widthSpin->setValue(640);

            m_heightSpin = new QSpinBox();
            m_heightSpin->setRange(1, 8192);
            m_heightSpin->setValue(480);

            layout->addWidget(new QLabel("W"));
            layout->addWidget(m_widthSpin);
            layout->addWidget(new QLabel("H"));
            layout->addWidget(m_heightSpin);

            connect(m_widthSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
            connect(m_heightSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
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
        cv::Mat dst;
        cv::resize(src, dst, cv::Size(m_widthSpin->value(), m_heightSpin->value()));
        m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
        m_output = std::make_shared<ImageData>(
            m_input->image().scaled(m_widthSpin->value(), m_heightSpin->value()));
#endif
        Q_EMIT dataUpdated(0);
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
