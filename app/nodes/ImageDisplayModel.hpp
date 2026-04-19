#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QLabel>
#include <QVBoxLayout>

#include "data/ImageData.hpp"

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
        m_receivedData = std::dynamic_pointer_cast<ImageData>(data);

        if (m_receivedData && !m_receivedData->image().isNull()) {
            m_preview->setPixmap(QPixmap::fromImage(
                m_receivedData->image().scaled(200, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        } else {
            m_receivedData.reset();
            m_preview->setText("No image");
        }
    }

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
            m_preview->setFixedSize(200, 150);
            m_preview->setAlignment(Qt::AlignCenter);
            m_preview->setStyleSheet("border: 1px solid #555; background: #222;");

            layout->addWidget(m_preview);
        }
        return m_widget;
    }

private:
    QWidget *m_widget = nullptr;
    QLabel *m_preview = nullptr;
    std::shared_ptr<ImageData> m_receivedData;
};
