#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "data/ImageData.hpp"
#include "plugins/PythonPluginRuntime.hpp"
#include "widgets/PipelineProfiler.hpp"

class PythonPluginModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    QString name() const override { return QStringLiteral("PythonPlugin"); }
    QString caption() const override { return QStringLiteral("Python Plugin"); }

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

        process();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override
    {
        return m_output;
    }

    QJsonObject save() const override
    {
        auto j = NodeDelegateModel::save();
        j["pluginPath"] = m_pluginPath;
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_pluginPath = j["pluginPath"].toString();
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            auto *row = new QHBoxLayout();
            m_pathEdit = new QLineEdit(m_pluginPath);
            m_pathEdit->setPlaceholderText("Select .py plugin...");
            auto *browse = new QPushButton("...");
            browse->setFixedWidth(28);
            row->addWidget(m_pathEdit);
            row->addWidget(browse);
            layout->addLayout(row);

            m_statusLabel = new QLabel("No plugin loaded");
            m_statusLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_statusLabel);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

            connect(m_pathEdit, &QLineEdit::editingFinished, this, [this]() {
                m_pluginPath = m_pathEdit->text().trimmed();
                loadPlugin();
                process();
            });
            connect(browse, &QPushButton::clicked, this, [this]() {
                QString fp = QFileDialog::getOpenFileName(
                    nullptr, "Select Python Plugin", QString(), "Python Files (*.py)");
                if (fp.isEmpty())
                    return;
                m_pluginPath = fp;
                m_pathEdit->setText(fp);
                loadPlugin();
                process();
            });

            loadPlugin();
        }
        return m_widget;
    }

    Q_INVOKABLE void refreshWidgets()
    {
        if (m_statusLabel)
            m_statusLabel->setText(m_statusText);
        if (m_timeLabel) {
            double ms = PipelineProfiler::instance().stat(
                caption(), "process", PipelineProfiler::StatType::Avg,
                PipelineProfiler::TimeWindow::Sec1);
            m_timeLabel->setText(QString("%1 ms").arg(ms, 0, 'f', 2));
        }
    }

private:
    void loadPlugin()
    {
        if (m_pluginPath.isEmpty()) {
            m_pluginLoaded = false;
            m_statusText = QStringLiteral("No plugin loaded");
            refreshWidgets();
            return;
        }

        QString err;
        m_pluginLoaded = m_runtime.setPluginFile(m_pluginPath, err);
        if (m_pluginLoaded) {
            QString n = m_runtime.pluginName();
            m_statusText = n.isEmpty() ? QStringLiteral("Plugin loaded")
                                       : QStringLiteral("Loaded: %1").arg(n);
        } else {
            m_statusText = err;
        }
        refreshWidgets();
    }

    void process()
    {
        if (!m_runtime.isAvailable()) {
            setValidationState({QtNodes::NodeValidationState::State::Error,
                                "Python support disabled in this build"});
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        if (!m_input || m_input->image().isNull()) {
            setValidationState({QtNodes::NodeValidationState::State::Warning, "Missing input"});
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        if (!m_pluginLoaded) {
            setValidationState({QtNodes::NodeValidationState::State::Error,
                                m_statusText.isEmpty() ? QStringLiteral("Plugin not loaded") : m_statusText});
            m_output.reset();
            if (!frozen()) Q_EMIT dataUpdated(0);
            return;
        }

        ScopeStageTimer t(caption(), "process");
        QString err;
        auto out = m_runtime.process(*m_input, err);
        if (!out.has_value()) {
            setValidationState({QtNodes::NodeValidationState::State::Error, err});
            m_statusText = err;
            m_output.reset();
            if (!frozen()) {
                Q_EMIT dataUpdated(0);
                QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
            }
            return;
        }

        setValidationState({QtNodes::NodeValidationState::State::Valid, ""});
        m_output = std::make_shared<ImageData>(out->image(), out->colorSpace());
        if (!frozen()) {
            Q_EMIT dataUpdated(0);
            QMetaObject::invokeMethod(this, "refreshWidgets", Qt::QueuedConnection);
        }
    }

    QWidget *m_widget = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_timeLabel = nullptr;

    noddle::PythonPluginRuntime m_runtime;
    QString m_pluginPath;
    QString m_statusText;
    bool m_pluginLoaded = false;

    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
