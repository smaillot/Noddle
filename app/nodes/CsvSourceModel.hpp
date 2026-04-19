#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <fstream>
#include <sstream>

#include "data/TableData.hpp"

class CsvSourceModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("CsvSource"); }
    QString caption() const override { return QStringLiteral("CSV Source"); }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::Out)
            return 1;
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override
    {
        return TableData({}).type();
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
            loadCsvFile(m_filePath);
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_tableData;
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

            auto *btn = new QPushButton("Load CSV...");
            connect(btn, &QPushButton::clicked, this, &CsvSourceModel::onLoadClicked);

            layout->addWidget(m_infoLabel);
            layout->addWidget(btn);

            if (m_tableData) {
                m_infoLabel->setText(
                    QString("%1 rows x %2 cols")
                        .arg(m_tableData->rowCount())
                        .arg(m_tableData->colCount()));
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
            m_widget, "Open CSV", QString(), "CSV files (*.csv)");

        if (path.isEmpty())
            return;

        m_filePath = path;
        loadCsvFile(m_filePath);

        if (m_tableData && m_infoLabel) {
            m_infoLabel->setText(
                QString("%1 rows x %2 cols")
                    .arg(m_tableData->rowCount())
                    .arg(m_tableData->colCount()));
        }

        Q_EMIT dataUpdated(0);
    }

private:
    void loadCsvFile(QString const &path)
    {
        std::ifstream file(path.toStdString());
        if (!file.is_open()) {
            setValidationState({QtNodes::NodeValidationState::State::Error, "Cannot open file"});
            return;
        }

        std::vector<std::vector<float>> rows;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;
            std::vector<float> row;
            std::istringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ','))
                row.push_back(std::stof(cell));
            rows.push_back(std::move(row));
        }

        m_tableData = std::make_shared<TableData>(std::move(rows));
        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
    }

    QWidget *m_widget = nullptr;
    QLabel *m_infoLabel = nullptr;
    QString m_filePath;
    std::shared_ptr<TableData> m_tableData;
};
