#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QImage>

#include "data/ImageData.hpp"

class ImageSourceModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("ImageSource"); }
    QString caption() const override { return QStringLiteral("Image Source"); }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::Out)
            return 1;
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return ImageData(QImage()).type();
    }

    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_imageData;
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_preview = new QLabel("No image");
            m_preview->setFixedSize(120, 90);
            m_preview->setAlignment(Qt::AlignCenter);
            m_preview->setStyleSheet("border: 1px solid #555; background: #222;");

            auto *btn = new QPushButton("Load...");
            connect(btn, &QPushButton::clicked, this, &ImageSourceModel::onLoadClicked);

            layout->addWidget(m_preview);
            layout->addWidget(btn);
        }
        return m_widget;
    }

private Q_SLOTS:
    void onLoadClicked()
    {
        QString path = QFileDialog::getOpenFileName(
            m_widget, "Open Image", QString(),
            "Images (*.png *.jpg *.jpeg *.bmp *.tiff)");

        if (path.isEmpty())
            return;

        QImage img(path);
        if (img.isNull())
            return;

        m_imageData = std::make_shared<ImageData>(img);
        m_preview->setPixmap(QPixmap::fromImage(
            img.scaled(120, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        Q_EMIT dataUpdated(0);
    }

private:
    QWidget *m_widget = nullptr;
    QLabel *m_preview = nullptr;
    std::shared_ptr<ImageData> m_imageData;
};
