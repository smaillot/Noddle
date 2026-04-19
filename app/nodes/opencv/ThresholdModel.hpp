#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"
#include "widgets/PipelineProfiler.hpp"

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
        j["threshValue"] = m_threshSpin ? m_threshSpin->value() : m_threshValue;
        j["typeIndex"] = m_typeCombo ? m_typeCombo->currentIndex() : m_typeIndex;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_threshValue = j["threshValue"].toInt(128);
        m_typeIndex = j["typeIndex"].toInt(0);
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_threshSpin = new QSpinBox();
            m_threshSpin->setRange(0, 255);
            m_threshSpin->setValue(m_threshValue);

            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"Binary", "Binary Inv", "Otsu"});
            m_typeCombo->setCurrentIndex(m_typeIndex);

            layout->addWidget(new QLabel("Threshold"));
            layout->addWidget(m_threshSpin);
            layout->addWidget(m_typeCombo);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

            connect(m_threshSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
            connect(m_typeCombo, &QComboBox::currentIndexChanged, this, [this]() { process(); });
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
            cv::Mat gray;
            if (src.channels() == 3)
                cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
            else
                gray = src;

            cv::Mat dst;
            int type = cv::THRESH_BINARY;
            int tidx = m_typeCombo ? m_typeCombo->currentIndex() : m_typeIndex;
            switch (tidx) {
            case 0: type = cv::THRESH_BINARY; break;
            case 1: type = cv::THRESH_BINARY_INV; break;
            case 2: type = cv::THRESH_BINARY | cv::THRESH_OTSU; break;
            }
            int tv = m_threshSpin ? m_threshSpin->value() : m_threshValue;
            cv::threshold(gray, dst, tv, 255, type);
            m_output = std::make_shared<ImageData>(matToQImage(dst));
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
    QSpinBox *m_threshSpin = nullptr;
    QComboBox *m_typeCombo = nullptr;
    QLabel *m_timeLabel = nullptr;
    int m_threshValue = 128;
    int m_typeIndex = 0;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
