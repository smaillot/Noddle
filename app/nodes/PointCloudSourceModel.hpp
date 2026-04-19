#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <fstream>
#include <sstream>

#include "data/PointCloudData.hpp"

class PointCloudSourceModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("PointCloudSource"); }
    QString caption() const override { return QStringLiteral("Point Cloud Source"); }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::Out)
            return 1;
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return PointCloudData({}).type();
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
        if (!m_filePath.isEmpty())
            loadPointCloudFile(m_filePath);
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_cloudData;
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            m_infoLabel = new QLabel("No file");
            m_infoLabel->setAlignment(Qt::AlignCenter);
            m_infoLabel->setStyleSheet("color: #aaa;");

            auto *btn = new QPushButton("Load XYZ...");
            connect(btn, &QPushButton::clicked, this, &PointCloudSourceModel::onLoadClicked);

            layout->addWidget(m_infoLabel);
            layout->addWidget(btn);

            if (m_cloudData) {
                m_infoLabel->setText(QString("%1 points").arg(m_cloudData->size()));
            } else {
                setValidationState({QtNodes::NodeValidationState::State::Warning, "No file loaded"});
            }
        }
        return m_widget;
    }

private Q_SLOTS:
    void onLoadClicked()
    {
        QString path = QFileDialog::getOpenFileName(
            m_widget, "Open Point Cloud CSV", QString(), "CSV files (*.csv)");

        if (path.isEmpty())
            return;

        m_filePath = path;
        loadPointCloudFile(m_filePath);

        if (m_cloudData && m_infoLabel)
            m_infoLabel->setText(QString("%1 points").arg(m_cloudData->size()));

        Q_EMIT dataUpdated(0);
    }

private:
    void loadPointCloudFile(QString const &path)
    {
        std::ifstream file(path.toStdString());
        if (!file.is_open()) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Cannot open file"});
            return;
        }

        std::vector<Point3D> points;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;
            std::istringstream ss(line);
            Point3D p{};
            char sep;
            if (ss >> p.x >> sep >> p.y >> sep >> p.z)
                points.push_back(p);
        }

        m_cloudData = std::make_shared<PointCloudData>(std::move(points));
        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
    }

    QWidget *m_widget = nullptr;
    QLabel *m_infoLabel = nullptr;
    QString m_filePath;
    std::shared_ptr<PointCloudData> m_cloudData;
};
