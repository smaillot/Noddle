#pragma once

#include <QString>

#include <optional>

#include "data/ImageData.hpp"

namespace noddle {

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

    std::optional<ImageData> process(ImageData const &input, QString &error);

private:
#ifdef NODDLE_WITH_PYTHON_PLUGIN
    struct Impl;
    Impl *m_impl = nullptr;
#endif

    QString m_pluginFile;
    QString m_pluginName;
};

} // namespace noddle
