#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#endif

class MorphologyModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("Morphology"); }
    QString caption() const override { return QStringLiteral("Morphology"); }

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

            m_opCombo = new QComboBox();
            m_opCombo->addItems({"Erode", "Dilate", "Open", "Close"});

            m_kernelSpin = new QSpinBox();
            m_kernelSpin->setRange(1, 31);
            m_kernelSpin->setSingleStep(2);
            m_kernelSpin->setValue(3);

            layout->addWidget(new QLabel("Operation"));
            layout->addWidget(m_opCombo);
            layout->addWidget(new QLabel("Kernel"));
            layout->addWidget(m_kernelSpin);

            connect(m_opCombo, &QComboBox::currentIndexChanged, this, [this]() { process(); });
            connect(m_kernelSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
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
        int k = m_kernelSpin->value() | 1;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
        cv::Mat dst;

        switch (m_opCombo->currentIndex()) {
        case 0: cv::erode(src, dst, kernel); break;
        case 1: cv::dilate(src, dst, kernel); break;
        case 2: cv::morphologyEx(src, dst, cv::MORPH_OPEN, kernel); break;
        case 3: cv::morphologyEx(src, dst, cv::MORPH_CLOSE, kernel); break;
        default: dst = src; break;
        }

        m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
        m_output = m_input;
#endif
        Q_EMIT dataUpdated(0);
    }

    QWidget *m_widget = nullptr;
    QComboBox *m_opCombo = nullptr;
    QSpinBox *m_kernelSpin = nullptr;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
