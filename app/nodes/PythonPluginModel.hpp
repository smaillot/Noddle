#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVariantMap>
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
        j["pluginFolderPath"] = m_pluginFolderPath;
        j["selectedPluginId"] = m_selectedPluginId;
        // Keep legacy pluginPath for direct-file load compatibility
        j["pluginPath"] = m_pluginPath;
        j["params"] = QJsonObject::fromVariantMap(m_paramValues);
        return j;
    }

    void load(QJsonObject const &j) override
    {
        m_pluginFolderPath = j["pluginFolderPath"].toString();
        m_selectedPluginId = j["selectedPluginId"].toString();
        // Legacy: direct plugin path (from previous save format)
        if (m_pluginFolderPath.isEmpty())
            m_pluginPath = j["pluginPath"].toString();
        m_paramValues = j["params"].toObject().toVariantMap();

        // Headless compatibility: load direct plugin path even if widget was not created yet.
        if (m_combo == nullptr && m_pluginFolderPath.isEmpty() && !m_pluginPath.isEmpty())
            loadPlugin();

        // Refresh UI if widget was already created before load() was called
        if (m_combo && !m_pluginFolderPath.isEmpty())
            scanPlugins();
        else if (m_combo && !m_pluginPath.isEmpty())
            loadPlugin();
    }

    QWidget *embeddedWidget() override
    {
        if (!m_widget) {
            m_widget = new QWidget();
            auto *layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);

            // ── Folder row ──
            auto *folderRow = new QHBoxLayout();
            m_folderEdit = new QLineEdit(m_pluginFolderPath);
            m_folderEdit->setPlaceholderText("Plugin folder...");
            auto *browseFolder = new QPushButton("...");
            browseFolder->setFixedWidth(28);
            folderRow->addWidget(m_folderEdit);
            folderRow->addWidget(browseFolder);
            layout->addLayout(folderRow);

            // ── Plugin combo ──
            m_combo = new QComboBox();
            m_combo->setObjectName("pluginCombo");
            m_combo->addItem("(select a plugin)", QString());
            layout->addWidget(m_combo);

            // ── Param editors ──
            m_paramsLayout = new QFormLayout();
            layout->addLayout(m_paramsLayout);

            // ── Status / timing ──
            m_statusLabel = new QLabel("No plugin loaded");
            m_statusLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_statusLabel);

            m_timeLabel = new QLabel("— ms");
            m_timeLabel->setStyleSheet("color: #aaa; font-size: 10px;");
            layout->addWidget(m_timeLabel);

            connect(m_folderEdit, &QLineEdit::editingFinished, this, [this]() {
                m_pluginFolderPath = m_folderEdit->text().trimmed();
                scanPlugins();
            });
            connect(browseFolder, &QPushButton::clicked, this, [this]() {
                QString dir = QFileDialog::getExistingDirectory(
                    nullptr, "Select Plugin Folder",
                    m_pluginFolderPath.isEmpty() ? defaultPluginFolder() : m_pluginFolderPath);
                if (dir.isEmpty())
                    return;
                m_pluginFolderPath = dir;
                m_folderEdit->setText(dir);
                scanPlugins();
            });
            connect(m_combo, &QComboBox::currentIndexChanged, this, [this](int index) {
                QString filePath = m_combo->itemData(index).toString();
                m_selectedPluginId = m_combo->itemData(index, Qt::UserRole + 1).toString();
                m_pluginPath = filePath;
                loadPlugin();
                process();
            });

            // Restore state after load()
            if (!m_pluginFolderPath.isEmpty())
                scanPlugins();
            else if (!m_pluginPath.isEmpty())
                loadPlugin(); // legacy: direct path support
        }
        return m_widget;
    }

    Q_INVOKABLE void refreshWidgets()
    {
        if (m_folderEdit)
            m_folderEdit->setText(m_pluginFolderPath);
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
    static QString defaultPluginFolder()
    {
        QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        return QDir(config).absoluteFilePath(QStringLiteral("plugins"));
    }

    void scanPlugins()
    {
        auto descriptors = noddle::PythonPluginRuntime::scanDirectory(m_pluginFolderPath);

        if (m_combo) {
            // Block signals while rebuilding to avoid spurious loadPlugin() calls
            QSignalBlocker blocker(m_combo);
            m_combo->clear();
            m_combo->addItem(QStringLiteral("(select a plugin)"), QString());
            for (auto const &desc : descriptors) {
                m_combo->addItem(desc.name.isEmpty() ? desc.id : desc.name, desc.filePath);
                m_combo->setItemData(m_combo->count() - 1, desc.id, Qt::UserRole + 1);
            }

            // Re-select previously chosen plugin by id
            if (!m_selectedPluginId.isEmpty()) {
                for (int i = 1; i < m_combo->count(); ++i) {
                    if (m_combo->itemData(i, Qt::UserRole + 1).toString() == m_selectedPluginId) {
                        m_combo->setCurrentIndex(i);
                        m_pluginPath = m_combo->itemData(i).toString();
                        loadPlugin();
                        return;
                    }
                }
            }
        }

        // No plugin selected after scan
        m_pluginPath.clear();
        loadPlugin();
    }

    void clearParamEditors()
    {
        if (!m_paramsLayout)
            return;
        while (m_paramsLayout->rowCount() > 0) {
            auto *labelItem = m_paramsLayout->itemAt(0, QFormLayout::LabelRole);
            auto *fieldItem = m_paramsLayout->itemAt(0, QFormLayout::FieldRole);
            if (labelItem) {
                if (auto *widget = labelItem->widget())
                    delete widget;
                delete labelItem;
            }
            if (fieldItem) {
                if (auto *widget = fieldItem->widget())
                    delete widget;
                delete fieldItem;
            }
            m_paramsLayout->removeRow(0);
        }
    }

    void rebuildParamEditors()
    {
        clearParamEditors();
        if (!m_paramsLayout)
            return;

        for (auto const &spec : m_runtime.parameterSpecs()) {
            if (!m_paramValues.contains(spec.name))
                m_paramValues.insert(spec.name, spec.defaultValue);

            QWidget *editor = nullptr;
            switch (spec.type) {
            case noddle::PythonPluginParamType::Integer: {
                auto *spin = new QSpinBox();
                spin->setObjectName(QStringLiteral("param_") + spec.name);
                spin->setRange(spec.minValue.isValid() ? spec.minValue.toInt() : -999999,
                               spec.maxValue.isValid() ? spec.maxValue.toInt() : 999999);
                spin->setValue(m_paramValues.value(spec.name, spec.defaultValue).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, spec](int value) {
                    m_paramValues.insert(spec.name, value);
                    process();
                });
                editor = spin;
                break;
            }
            case noddle::PythonPluginParamType::Double: {
                auto *spin = new QDoubleSpinBox();
                spin->setObjectName(QStringLiteral("param_") + spec.name);
                spin->setRange(spec.minValue.isValid() ? spec.minValue.toDouble() : -999999.0,
                               spec.maxValue.isValid() ? spec.maxValue.toDouble() : 999999.0);
                spin->setSingleStep(spec.stepValue.isValid() ? spec.stepValue.toDouble() : 0.1);
                spin->setDecimals(3);
                spin->setValue(m_paramValues.value(spec.name, spec.defaultValue).toDouble());
                connect(spin, &QDoubleSpinBox::valueChanged, this, [this, spec](double value) {
                    m_paramValues.insert(spec.name, value);
                    process();
                });
                editor = spin;
                break;
            }
            case noddle::PythonPluginParamType::Boolean: {
                auto *check = new QCheckBox();
                check->setObjectName(QStringLiteral("param_") + spec.name);
                check->setChecked(m_paramValues.value(spec.name, spec.defaultValue).toBool());
                connect(check, &QCheckBox::toggled, this, [this, spec](bool checked) {
                    m_paramValues.insert(spec.name, checked);
                    process();
                });
                editor = check;
                break;
            }
            case noddle::PythonPluginParamType::String: {
                auto *lineEdit = new QLineEdit(m_paramValues.value(spec.name, spec.defaultValue).toString());
                lineEdit->setObjectName(QStringLiteral("param_") + spec.name);
                connect(lineEdit, &QLineEdit::editingFinished, this, [this, spec, lineEdit]() {
                    m_paramValues.insert(spec.name, lineEdit->text());
                    process();
                });
                editor = lineEdit;
                break;
            }
            }

            auto *label = new QLabel(spec.label);
            m_paramsLayout->addRow(label, editor);
        }
    }

    void loadPlugin()
    {
        if (m_pluginPath.isEmpty()) {
            m_pluginLoaded = false;
            m_statusText = QStringLiteral("No plugin loaded");
            clearParamEditors();
            refreshWidgets();
            return;
        }

        QString err;
        m_pluginLoaded = m_runtime.setPluginFile(m_pluginPath, err);
        if (m_pluginLoaded) {
            QVariantMap defaults = m_runtime.defaultParameters();
            for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
                if (!m_paramValues.contains(it.key()))
                    m_paramValues.insert(it.key(), it.value());
            }
            QString n = m_runtime.pluginName();
            m_statusText = n.isEmpty() ? QStringLiteral("Plugin loaded")
                                       : QStringLiteral("Loaded: %1").arg(n);
            rebuildParamEditors();
        } else {
            m_statusText = err;
            clearParamEditors();
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
        auto out = m_runtime.process(*m_input, m_paramValues, err);
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
    QFormLayout *m_paramsLayout = nullptr;
    QLineEdit *m_folderEdit = nullptr;
    QComboBox *m_combo = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_timeLabel = nullptr;

    noddle::PythonPluginRuntime m_runtime;
    QString m_pluginFolderPath;
    QString m_selectedPluginId;
    QString m_pluginPath;   // resolved absolute path of selected plugin
    QString m_statusText;
    bool m_pluginLoaded = false;
    QVariantMap m_paramValues;

    std::shared_ptr<ImageData> m_input;
    std::shared_ptr<ImageData> m_output;
};
