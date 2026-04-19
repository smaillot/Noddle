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

class ThresholdModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("Threshold"); }
    QString caption() const override { return QStringLiteral("Threshold"); }

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

            m_threshSpin = new QSpinBox();
            m_threshSpin->setRange(0, 255);
            m_threshSpin->setValue(128);

            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"Binary", "Binary Inv", "Otsu"});

            layout->addWidget(new QLabel("Threshold"));
            layout->addWidget(m_threshSpin);
            layout->addWidget(m_typeCombo);

            connect(m_threshSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
            connect(m_typeCombo, &QComboBox::currentIndexChanged, this, [this]() { process(); });
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
        int type = cv::THRESH_BINARY;
        if (m_typeCombo) {
            switch (m_typeCombo->currentIndex()) {
            case 0: type = cv::THRESH_BINARY; break;
            case 1: type = cv::THRESH_BINARY_INV; break;
            case 2: type = cv::THRESH_BINARY | cv::THRESH_OTSU; break;
            }
        }
        cv::threshold(gray, dst, m_threshSpin->value(), 255, type);
        m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
        m_output = m_input;
#endif
        Q_EMIT dataUpdated(0);
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_threshSpin = nullptr;
    QComboBox *m_typeCombo = nullptr;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
