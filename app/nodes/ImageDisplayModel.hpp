#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QLabel>
#include <QVBoxLayout>

#include "data/ImageData.hpp"
#include "widgets/PipelineProfiler.hpp"

class ImageDisplayModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("ImageDisplay"); }
    QString caption() const override { return QStringLiteral("Image Display"); }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::In)
            return 1;
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return ImageData(QImage()).type();
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex) override
    {
        if (!data) {
            setValidationState({QtNodes::NodeValidationState::State::Warning, "No input image"});
            m_receivedData.reset();
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
            return;
        }
        m_receivedData = std::dynamic_pointer_cast<ImageData>(data);
        if (!m_receivedData) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Type mismatch"});
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
            return;
        }
        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
        QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
    }

    // Return received data so PreviewPanel can display sink nodes
    // (no output ports, but outData(0) exposes the last input)
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_receivedData;
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_preview = new QLabel("No image");
            m_preview->setObjectName("nodePreview");
            m_preview->setFixedSize(200, 150);
            m_preview->setAlignment(Qt::AlignCenter);
            m_preview->setStyleSheet("border: 1px solid #555; background: #222;");

            layout->addWidget(m_preview);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);
        }
        return m_widget;
    }

    Q_INVOKABLE void refreshWidgets()
    {
        embeddedWidget();

        if (!m_receivedData || m_receivedData->image().isNull()) {
            if (m_preview)
                m_preview->setText("No image");
            return;
        }
        {
            ScopeStageTimer t(caption(), "display");
            QImage scaled = m_receivedData->image().scaled(
                200, 150, Qt::KeepAspectRatio, Qt::FastTransformation);
            m_preview->setPixmap(QPixmap::fromImage(scaled));
        }
        if (m_timeLabel) {
            double ms = PipelineProfiler::instance().stat(
                caption(), "display",
                PipelineProfiler::StatType::Avg, PipelineProfiler::TimeWindow::Sec1);
            m_timeLabel->setText(QString("%1 ms").arg(ms, 0, 'f', 2));
        }
    }

private:
    QWidget *m_widget = nullptr;
    QLabel *m_preview = nullptr;
    QLabel *m_timeLabel = nullptr;
    std::shared_ptr<ImageData> m_receivedData;
};
