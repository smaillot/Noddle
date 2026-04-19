#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"
#include "widgets/PipelineProfiler.hpp"

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
        j["width"] = m_widthSpin ? m_widthSpin->value() : m_width;
        j["height"] = m_heightSpin ? m_heightSpin->value() : m_height;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_width = j["width"].toInt(640);
        m_height = j["height"].toInt(480);
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *mainLayout = new QVBoxLayout(m_widget);
            mainLayout->setContentsMargins(4, 4, 4, 4);

            auto *sizeLayout = new QHBoxLayout();

            m_widthSpin = new QSpinBox();
            m_widthSpin->setRange(1, 8192);
            m_widthSpin->setValue(m_width);

            m_heightSpin = new QSpinBox();
            m_heightSpin->setRange(1, 8192);
            m_heightSpin->setValue(m_height);

            sizeLayout->addWidget(new QLabel("W"));
            sizeLayout->addWidget(m_widthSpin);
            sizeLayout->addWidget(new QLabel("H"));
            sizeLayout->addWidget(m_heightSpin);
            mainLayout->addLayout(sizeLayout);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            mainLayout->addWidget(m_timeLabel);

            connect(m_widthSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
            connect(m_heightSpin, &QSpinBox::valueChanged, this, [this]() { process(); });
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
            int w = m_widthSpin ? m_widthSpin->value() : m_width;
            int h = m_heightSpin ? m_heightSpin->value() : m_height;
            cv::resize(src, dst, cv::Size(w, h));
            m_output = std::make_shared<ImageData>(matToQImage(dst));
#else
            int w = m_widthSpin ? m_widthSpin->value() : m_width;
            int h = m_heightSpin ? m_heightSpin->value() : m_height;
            m_output = std::make_shared<ImageData>(
                m_input->image().scaled(w, h));
#endif
        }
        if (!frozen()) {
            Q_EMIT dataUpdated(0);
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
        }
    }

    QWidget *m_widget = nullptr;
    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    QLabel *m_timeLabel = nullptr;
    int m_width = 640;
    int m_height = 480;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
