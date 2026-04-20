#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>

#include "data/ImageData.hpp"
#include "widgets/PipelineProfiler.hpp"

#ifdef NODDLE_WITH_OPENCV
#include "nodes/opencv/MatConvert.hpp"
#include <opencv2/imgproc.hpp>
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

    // Returns the list of valid target color space names for a given input space
    static QStringList validTargets(ColorSpace from)
    {
        switch (from) {
        case ColorSpace::RGB:
        case ColorSpace::BGR:
            return {"RGB", "BGR", "Gray", "HSV", "HLS", "Lab", "YCrCb", "XYZ"};
        case ColorSpace::Grayscale:
            return {"RGB", "BGR"};
        case ColorSpace::HSV:
            return {"RGB", "BGR"};
        case ColorSpace::HLS:
            return {"RGB", "BGR"};
        case ColorSpace::Lab:
            return {"RGB", "BGR"};
        case ColorSpace::YCrCb:
            return {"RGB", "BGR"};
        case ColorSpace::XYZ:
            return {"RGB", "BGR"};
        }
        return {};
    }

    // Returns the OpenCV color conversion code, or -1 if invalid
    static int cvtColorCode(ColorSpace from, ColorSpace to)
    {
#ifdef NODDLE_WITH_OPENCV
        if (from == to) return -1;
        // RGB source
        if (from == ColorSpace::RGB && to == ColorSpace::BGR)       return cv::COLOR_RGB2BGR;
        if (from == ColorSpace::RGB && to == ColorSpace::Grayscale) return cv::COLOR_RGB2GRAY;
        if (from == ColorSpace::RGB && to == ColorSpace::HSV)       return cv::COLOR_RGB2HSV;
        if (from == ColorSpace::RGB && to == ColorSpace::HLS)       return cv::COLOR_RGB2HLS;
        if (from == ColorSpace::RGB && to == ColorSpace::Lab)       return cv::COLOR_RGB2Lab;
        if (from == ColorSpace::RGB && to == ColorSpace::YCrCb)     return cv::COLOR_RGB2YCrCb;
        if (from == ColorSpace::RGB && to == ColorSpace::XYZ)       return cv::COLOR_RGB2XYZ;
        // BGR source
        if (from == ColorSpace::BGR && to == ColorSpace::RGB)       return cv::COLOR_BGR2RGB;
        if (from == ColorSpace::BGR && to == ColorSpace::Grayscale) return cv::COLOR_BGR2GRAY;
        if (from == ColorSpace::BGR && to == ColorSpace::HSV)       return cv::COLOR_BGR2HSV;
        if (from == ColorSpace::BGR && to == ColorSpace::HLS)       return cv::COLOR_BGR2HLS;
        if (from == ColorSpace::BGR && to == ColorSpace::Lab)       return cv::COLOR_BGR2Lab;
        if (from == ColorSpace::BGR && to == ColorSpace::YCrCb)     return cv::COLOR_BGR2YCrCb;
        if (from == ColorSpace::BGR && to == ColorSpace::XYZ)       return cv::COLOR_BGR2XYZ;
        // Grayscale source
        if (from == ColorSpace::Grayscale && to == ColorSpace::RGB) return cv::COLOR_GRAY2RGB;
        if (from == ColorSpace::Grayscale && to == ColorSpace::BGR) return cv::COLOR_GRAY2BGR;
        // HSV source
        if (from == ColorSpace::HSV && to == ColorSpace::RGB)       return cv::COLOR_HSV2RGB;
        if (from == ColorSpace::HSV && to == ColorSpace::BGR)       return cv::COLOR_HSV2BGR;
        // HLS source
        if (from == ColorSpace::HLS && to == ColorSpace::RGB)       return cv::COLOR_HLS2RGB;
        if (from == ColorSpace::HLS && to == ColorSpace::BGR)       return cv::COLOR_HLS2BGR;
        // Lab source
        if (from == ColorSpace::Lab && to == ColorSpace::RGB)       return cv::COLOR_Lab2RGB;
        if (from == ColorSpace::Lab && to == ColorSpace::BGR)       return cv::COLOR_Lab2BGR;
        // YCrCb source
        if (from == ColorSpace::YCrCb && to == ColorSpace::RGB)     return cv::COLOR_YCrCb2RGB;
        if (from == ColorSpace::YCrCb && to == ColorSpace::BGR)     return cv::COLOR_YCrCb2BGR;
        // XYZ source
        if (from == ColorSpace::XYZ && to == ColorSpace::RGB)       return cv::COLOR_XYZ2RGB;
        if (from == ColorSpace::XYZ && to == ColorSpace::BGR)       return cv::COLOR_XYZ2BGR;
#else
        Q_UNUSED(from); Q_UNUSED(to);
#endif
        return -1;
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

        // Auto-follow incoming color space
        m_inputCS = m_input->colorSpace();
        if (m_inputCombo) {
            m_inputCombo->blockSignals(true);
            m_inputCombo->setCurrentIndex(static_cast<int>(m_inputCS));
            m_inputCombo->blockSignals(false);
        }
        rebuildOutputCombo();
        process();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_output;
    }

    QJsonObject save() const override
    {
        auto j = NodeDelegateModel::save();
        j["inputIndex"] = m_inputCombo ? m_inputCombo->currentIndex() : static_cast<int>(m_inputCS);
        j["outputIndex"] = m_outputCombo ? m_outputCombo->currentIndex() : m_outputIndex;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_inputCS = static_cast<ColorSpace>(j["inputIndex"].toInt(0));
        m_outputIndex = j["outputIndex"].toInt(0);
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_inputCombo = new QComboBox();
            for (int i = 0; i < colorSpaceCount(); ++i)
                m_inputCombo->addItem(colorSpaceName(static_cast<ColorSpace>(i)));
            m_inputCombo->setCurrentIndex(static_cast<int>(m_inputCS));
            layout->addWidget(m_inputCombo);

            m_outputCombo = new QComboBox();
            rebuildOutputCombo();
            layout->addWidget(m_outputCombo);

            m_timeLabel = new QLabel("\u2014 ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

            connect(m_inputCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
                m_inputCS = static_cast<ColorSpace>(idx);
                rebuildOutputCombo();
                process();
            });
            connect(m_outputCombo, &QComboBox::currentIndexChanged, this, [this]() { process(); });
        }
        return m_widget;
    }

    Q_INVOKABLE void refreshWidgets()
    {
        if (m_inputCombo)
            m_inputCombo->setCurrentIndex(static_cast<int>(m_inputCS));
        rebuildOutputCombo();
        if (m_timeLabel) {
            double ms = PipelineProfiler::instance().stat(
                caption(), "process",
                PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::Sec1);
            m_timeLabel->setText(QString("%1 ms").arg(ms, 0, 'f', 2));
        }
    }

private:
    ColorSpace selectedOutputCS() const
    {
        if (!m_outputCombo || m_outputCombo->count() == 0)
            return m_inputCS; // fallback: identity
        QString name = m_outputCombo->currentText();
        for (int i = 0; i < colorSpaceCount(); ++i) {
            auto cs = static_cast<ColorSpace>(i);
            if (name == colorSpaceName(cs))
                return cs;
        }
        return m_inputCS;
    }

    void rebuildOutputCombo()
    {
        if (!m_outputCombo) return;
        m_outputCombo->blockSignals(true);
        m_outputCombo->clear();
        m_outputCombo->addItems(validTargets(m_inputCS));
        if (m_outputIndex >= 0 && m_outputIndex < m_outputCombo->count())
            m_outputCombo->setCurrentIndex(m_outputIndex);
        else if (m_outputCombo->count() > 0)
            m_outputCombo->setCurrentIndex(0);
        m_outputCombo->blockSignals(false);
    }

    void process()
    {
        if (!m_input || m_input->image().isNull()) {
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        ColorSpace outCS = selectedOutputCS();

        // Pass-through if same space
        if (m_inputCS == outCS) {
            m_output = m_input;
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        {
            ScopeStageTimer t(caption(), "process");
#ifdef NODDLE_WITH_OPENCV
            int code = cvtColorCode(m_inputCS, outCS);
            if (code < 0) {
                m_output = m_input;
            } else {
                cv::Mat src = qImageToMat(m_input->image());
                cv::Mat dst;
                cv::cvtColor(src, dst, code);
                m_output = std::make_shared<ImageData>(matToQImage(dst), outCS);
            }
#else
            m_output = std::make_shared<ImageData>(m_input->image(), outCS);
#endif
        }
        if (!frozen()) {
            Q_EMIT dataUpdated(0);
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
        }
    }

    QWidget *m_widget = nullptr;
    QComboBox *m_inputCombo = nullptr;
    QComboBox *m_outputCombo = nullptr;
    QLabel *m_timeLabel = nullptr;
    ColorSpace m_inputCS = ColorSpace::RGB;
    int m_outputIndex = 0;
    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
