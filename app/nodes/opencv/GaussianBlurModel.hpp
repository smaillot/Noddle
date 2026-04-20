#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"
#include "widgets/PipelineProfiler.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#endif

class GaussianBlurModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("GaussianBlur"); }
    QString caption() const override { return QStringLiteral("Gaussian Blur"); }

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
        if (!data) {
            setValidationState({QtNodes::NodeValidationState::State::Warning, "Missing input"});
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }
        if (!m_input) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Type mismatch"});
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }
        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
        process();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_output;
    }

    QJsonObject save() const override
    {
        auto j = NodeDelegateModel::save();
        j["kernelSize"] = m_kernelSpin ? m_kernelSpin->value() : m_kernelSize;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_kernelSize = j["kernelSize"].toInt(5);
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_kernelSpin = new QSpinBox();
            m_kernelSpin->setRange(1, 99);
            m_kernelSpin->setSingleStep(2);
            m_kernelSpin->setValue(m_kernelSize);

            layout->addWidget(new QLabel("Kernel"));
            layout->addWidget(m_kernelSpin);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

            connect(m_kernelSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
        }
        return m_widget;
    }

    Q_INVOKABLE void refreshWidgets()
    {
        if (m_timeLabel) {
            double ms = PipelineProfiler::instance().stat(
                caption(), "process",
                PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::Sec1);
            m_timeLabel->setText(QString("%1 ms").arg(ms, 0, 'f', 2));
        }
    }

private:
    void process()
    {
        if (!m_input || m_input->image().isNull()) {
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        {
            ScopeStageTimer t(caption(), "process");
#ifdef NODDLE_WITH_OPENCV
            cv::Mat src = qImageToMat(m_input->image());
            cv::Mat dst;
            int k = (m_kernelSpin ? m_kernelSpin->value() : m_kernelSize) | 1;
            cv::GaussianBlur(src, dst, cv::Size(k, k), 0);
            m_output = std::make_shared<ImageData>(matToQImage(dst), m_input->colorSpace());
#else
            m_output = m_input;
#endif
        }
        if (!frozen()) {
            Q_EMIT dataUpdated(0);
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
        }
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_kernelSpin = nullptr;
    QLabel *m_timeLabel = nullptr;
    int m_kernelSize = 5;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
