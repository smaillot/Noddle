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
        if (!data) {
            setValidationState({QtNodes::NodeValidationState::State::Warning, "Missing input"});
            m_output.reset();
            Q_EMIT dataUpdated(0);
            return;
        }
        if (!m_input) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Type mismatch"});
            m_output.reset();
            Q_EMIT dataUpdated(0);
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
        j["lowThresh"] = m_lowSpin ? m_lowSpin->value() : m_lowThresh;
        j["highThresh"] = m_highSpin ? m_highSpin->value() : m_highThresh;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_lowThresh = j["lowThresh"].toInt(50);
        m_highThresh = j["highThresh"].toInt(150);
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_lowSpin = new QSpinBox();
            m_lowSpin->setRange(0, 500);
            m_lowSpin->setValue(m_lowThresh);

            m_highSpin = new QSpinBox();
            m_highSpin->setRange(0, 500);
            m_highSpin->setValue(m_highThresh);

            layout->addWidget(new QLabel("Low"));
            layout->addWidget(m_lowSpin);
            layout->addWidget(new QLabel("High"));
            layout->addWidget(m_highSpin);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

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

        {
            ScopeStageTimer t(caption(), "process");
#ifdef NODDLE_WITH_OPENCV
            cv::Mat src = qImageToMat(m_input->image());
            cv::Mat gray;
            if (src.channels() == 3)
                cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
            else
                gray = src;

            cv::Mat dst;
            int lo = m_lowSpin ? m_lowSpin->value() : m_lowThresh;
            int hi = m_highSpin ? m_highSpin->value() : m_highThresh;
            cv::Canny(gray, dst, lo, hi);
            m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
            m_output = m_input;
#endif
        }
        if (m_timeLabel) {
            double ms = PipelineProfiler::instance().stat(
                caption(), "process",
                PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::Sec1);
            m_timeLabel->setText(QString("%1 ms").arg(ms, 0, 'f', 2));
        }
        Q_EMIT dataUpdated(0);
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_lowSpin = nullptr;
    QSpinBox *m_highSpin = nullptr;
    QLabel *m_timeLabel = nullptr;
    int m_lowThresh = 50;
    int m_highThresh = 150;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
