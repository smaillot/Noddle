#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
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

    QJsonObject save() const override
    {
        auto j = NodeDelegateModel::save();
        j["filePath"] = m_filePath;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_filePath = j["filePath"].toString();
        if (!m_filePath.isEmpty()) {
            QImage img(m_filePath);
            if (!img.isNull()) {
                m_imageData = std::make_shared<ImageData>(img);
                setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
            } else {
                setValidationState({QtNodes::NodeValidationState::State::Error, "Cannot read file"});
            }
        }
    }

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

            // Restore preview from loaded data
            if (m_imageData && !m_imageData->image().isNull()) {
                m_preview->setPixmap(QPixmap::fromImage(
                    m_imageData->image().scaled(120, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                setValidationState({QtNodes::NodeValidationState::State::Warning, "No image loaded"});
            }
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
        if (img.isNull()) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Cannot read file"});
            return;
        }

        m_filePath = path;
        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});

        m_imageData = std::make_shared<ImageData>(img);
        m_preview->setPixmap(QPixmap::fromImage(
            img.scaled(120, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        Q_EMIT dataUpdated(0);
    }

private:
    QWidget *m_widget = nullptr;
    QLabel *m_preview = nullptr;
    QString m_filePath;
    std::shared_ptr<ImageData> m_imageData;
};
