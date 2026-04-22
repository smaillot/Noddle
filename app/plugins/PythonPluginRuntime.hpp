#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

#include <optional>

#include "data/ImageData.hpp"

namespace noddle {

struct PluginDescriptor {
    QString id;
    QString name;
    QString filePath;
};

enum class PythonPluginParamType {
    Integer,
    Double,
    Boolean,
    String,
};

struct PythonPluginParamSpec {
    QString name;
    QString label;
    PythonPluginParamType type = PythonPluginParamType::String;
    QVariant defaultValue;
    QVariant minValue;
    QVariant maxValue;
    QVariant stepValue;
};

class PythonPluginRuntime
{
public:
    PythonPluginRuntime();
    ~PythonPluginRuntime();

    PythonPluginRuntime(PythonPluginRuntime const &) = delete;
    PythonPluginRuntime &operator=(PythonPluginRuntime const &) = delete;

    bool isAvailable() const;

    bool setPluginFile(QString const &filePath, QString &error);
    QString pluginFile() const { return m_pluginFile; }
    QString pluginName() const { return m_pluginName; }
    QString pluginId() const { return m_pluginId; }
    QVector<PythonPluginParamSpec> parameterSpecs() const { return m_paramSpecs; }
    QVariantMap defaultParameters() const;

    std::optional<ImageData> process(ImageData const &input, QString &error);
    std::optional<ImageData> process(ImageData const &input, QVariantMap const &params, QString &error);

    static QVector<PluginDescriptor> scanDirectory(QString const &dirPath);

private:
#ifdef NODDLE_WITH_PYTHON_PLUGIN
    struct Impl;
    Impl *m_impl = nullptr;
#endif

    QString m_pluginFile;
    QString m_pluginId;
    QString m_pluginName;
    QVector<PythonPluginParamSpec> m_paramSpecs;
};

} // namespace noddle
