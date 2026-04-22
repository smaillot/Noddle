#include "plugins/PythonPluginRuntime.hpp"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#ifdef NODDLE_WITH_PYTHON_PLUGIN
#ifdef slots
#undef slots
#endif
#include <Python.h>

namespace {
QMutex &pythonMutex()
{
    static QMutex s_mutex;
    return s_mutex;
}

QString pyErrorToString()
{
    PyObject *ptype = nullptr;
    PyObject *pvalue = nullptr;
    PyObject *ptrace = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptrace);
    PyErr_NormalizeException(&ptype, &pvalue, &ptrace);

    QString out = QStringLiteral("Unknown Python error");

    if (pvalue) {
        PyObject *s = PyObject_Str(pvalue);
        if (s) {
            out = QString::fromUtf8(PyUnicode_AsUTF8(s));
            Py_DECREF(s);
        }
    }

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptrace);
    return out;
}

ColorSpace parseColorSpace(QString const &name)
{
    if (name.compare(QStringLiteral("BGR"), Qt::CaseInsensitive) == 0)
        return ColorSpace::BGR;
    if (name.compare(QStringLiteral("GRAY"), Qt::CaseInsensitive) == 0 ||
        name.compare(QStringLiteral("Grayscale"), Qt::CaseInsensitive) == 0)
        return ColorSpace::Grayscale;
    return ColorSpace::RGB;
}

QString colorSpaceString(ColorSpace cs)
{
    return QString::fromUtf8(colorSpaceName(cs));
}

QVariant pyObjectToVariant(PyObject *obj)
{
    if (!obj || obj == Py_None)
        return {};
    if (PyBool_Check(obj))
        return obj == Py_True;
    if (PyLong_Check(obj))
        return static_cast<int>(PyLong_AsLong(obj));
    if (PyFloat_Check(obj))
        return PyFloat_AsDouble(obj);
    if (PyUnicode_Check(obj))
        return QString::fromUtf8(PyUnicode_AsUTF8(obj));
    return {};
}

PyObject *variantToPyObject(QVariant const &value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
        return PyBool_FromLong(value.toBool() ? 1 : 0);
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
        return PyLong_FromLongLong(value.toLongLong());
    case QMetaType::Float:
    case QMetaType::Double:
        return PyFloat_FromDouble(value.toDouble());
    default: {
        QByteArray utf8 = value.toString().toUtf8();
        return PyUnicode_FromString(utf8.constData());
    }
    }
}

noddle::PythonPluginParamType parseParamType(QString const &typeName)
{
    if (typeName.compare(QStringLiteral("int"), Qt::CaseInsensitive) == 0)
        return noddle::PythonPluginParamType::Integer;
    if (typeName.compare(QStringLiteral("float"), Qt::CaseInsensitive) == 0 ||
        typeName.compare(QStringLiteral("double"), Qt::CaseInsensitive) == 0)
        return noddle::PythonPluginParamType::Double;
    if (typeName.compare(QStringLiteral("bool"), Qt::CaseInsensitive) == 0 ||
        typeName.compare(QStringLiteral("boolean"), Qt::CaseInsensitive) == 0)
        return noddle::PythonPluginParamType::Boolean;
    return noddle::PythonPluginParamType::String;
}

bool parseParamSpecs(PyObject *paramsObj, QVector<noddle::PythonPluginParamSpec> &specs, QString &error)
{
    specs.clear();
    if (!paramsObj || paramsObj == Py_None)
        return true;
    if (!PyDict_Check(paramsObj)) {
        error = QStringLiteral("Plugin contract error: params must be a dict");
        return false;
    }

    PyObject *key = nullptr;
    PyObject *value = nullptr;
    Py_ssize_t pos = 0;
    while (PyDict_Next(paramsObj, &pos, &key, &value)) {
        if (!PyUnicode_Check(key) || !PyDict_Check(value)) {
            error = QStringLiteral("Plugin contract error: params entries must be name -> dict");
            return false;
        }

        noddle::PythonPluginParamSpec spec;
        spec.name = QString::fromUtf8(PyUnicode_AsUTF8(key));
        spec.label = spec.name;

        PyObject *typeObj = PyDict_GetItemString(value, "type");
        if (!typeObj || !PyUnicode_Check(typeObj)) {
            error = QStringLiteral("Plugin contract error: param '%1' missing string type").arg(spec.name);
            return false;
        }
        spec.type = parseParamType(QString::fromUtf8(PyUnicode_AsUTF8(typeObj)));

        PyObject *labelObj = PyDict_GetItemString(value, "label");
        if (labelObj && PyUnicode_Check(labelObj))
            spec.label = QString::fromUtf8(PyUnicode_AsUTF8(labelObj));

        spec.defaultValue = pyObjectToVariant(PyDict_GetItemString(value, "default"));
        spec.minValue = pyObjectToVariant(PyDict_GetItemString(value, "min"));
        spec.maxValue = pyObjectToVariant(PyDict_GetItemString(value, "max"));
        spec.stepValue = pyObjectToVariant(PyDict_GetItemString(value, "step"));

        if (!spec.defaultValue.isValid()) {
            switch (spec.type) {
            case noddle::PythonPluginParamType::Integer: spec.defaultValue = 0; break;
            case noddle::PythonPluginParamType::Double: spec.defaultValue = 0.0; break;
            case noddle::PythonPluginParamType::Boolean: spec.defaultValue = false; break;
            case noddle::PythonPluginParamType::String: spec.defaultValue = QString(); break;
            }
        }
        specs.push_back(spec);
    }

    return true;
}

QImage normalizeInputImage(QImage const &src, ColorSpace cs)
{
    if (src.isNull())
        return {};

    if (cs == ColorSpace::Grayscale)
        return src.convertToFormat(QImage::Format_Grayscale8);

    return src.convertToFormat(QImage::Format_RGB888);
}
} // namespace
#endif

namespace noddle {

#ifdef NODDLE_WITH_PYTHON_PLUGIN
struct PythonPluginRuntime::Impl {
    PyObject *module = nullptr;
};
#endif

PythonPluginRuntime::PythonPluginRuntime()
{
#ifdef NODDLE_WITH_PYTHON_PLUGIN
    QMutexLocker lock(&pythonMutex());
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    m_impl = new Impl();
#endif
}

PythonPluginRuntime::~PythonPluginRuntime()
{
#ifdef NODDLE_WITH_PYTHON_PLUGIN
    QMutexLocker lock(&pythonMutex());
    PyGILState_STATE gil = PyGILState_Ensure();
    if (m_impl && m_impl->module) {
        Py_DECREF(m_impl->module);
        m_impl->module = nullptr;
    }
    PyGILState_Release(gil);
    delete m_impl;
    m_impl = nullptr;
#endif
}

bool PythonPluginRuntime::isAvailable() const
{
#ifdef NODDLE_WITH_PYTHON_PLUGIN
    return true;
#else
    return false;
#endif
}

bool PythonPluginRuntime::setPluginFile(QString const &filePath, QString &error)
{
    error.clear();
    m_pluginId.clear();
    m_pluginName.clear();
    m_paramSpecs.clear();

#ifndef NODDLE_WITH_PYTHON_PLUGIN
    Q_UNUSED(filePath)
    error = QStringLiteral("Python plugin support is disabled at build time");
    return false;
#else
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        error = QStringLiteral("Plugin file not found: %1").arg(filePath);
        return false;
    }

    QByteArray pathBytes = fi.absoluteFilePath().toUtf8();

    QMutexLocker lock(&pythonMutex());
    PyGILState_STATE gil = PyGILState_Ensure();

    PyObject *builtins = PyEval_GetBuiltins();
    PyObject *compileFn = PyDict_GetItemString(builtins, "compile");
    Q_UNUSED(compileFn)

    PyObject *globals = PyDict_New();
    PyObject *locals = PyDict_New();
    PyDict_SetItemString(globals, "__builtins__", builtins);
    PyObject *pathObj = PyUnicode_FromString(pathBytes.constData());
    PyDict_SetItemString(globals, "_noddle_plugin_path", pathObj);
    Py_DECREF(pathObj);

    char const *loadScript =
        "import importlib.util, hashlib\n"
        "_p = _noddle_plugin_path\n"
        "_name = 'noddle_plugin_' + hashlib.sha1(_p.encode('utf-8')).hexdigest()\n"
        "_spec = importlib.util.spec_from_file_location(_name, _p)\n"
        "if _spec is None or _spec.loader is None:\n"
        "    raise RuntimeError(f'Unable to create spec for {_p}')\n"
        "_mod = importlib.util.module_from_spec(_spec)\n"
        "_spec.loader.exec_module(_mod)\n"
        "_noddle_loaded_module = _mod\n";

    PyObject *execRes = PyRun_String(loadScript, Py_file_input, globals, locals);
    if (!execRes) {
        error = QStringLiteral("Plugin load error: %1").arg(pyErrorToString());
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }
    Py_DECREF(execRes);

    PyObject *module = PyDict_GetItemString(locals, "_noddle_loaded_module");
    if (!module) {
        error = QStringLiteral("Plugin load error: module object not found");
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    // Validate contract: plugin_spec() and process()
    PyObject *pluginSpecFn = PyObject_GetAttrString(module, "plugin_spec");
    if (!pluginSpecFn || !PyCallable_Check(pluginSpecFn)) {
        Py_XDECREF(pluginSpecFn);
        error = QStringLiteral("Plugin contract error: missing callable plugin_spec()");
        PyErr_Clear();
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    PyObject *processFn = PyObject_GetAttrString(module, "process");
    if (!processFn || !PyCallable_Check(processFn)) {
        Py_XDECREF(processFn);
        Py_DECREF(pluginSpecFn);
        error = QStringLiteral("Plugin contract error: missing callable process(...)");
        PyErr_Clear();
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    PyObject *specObj = PyObject_CallObject(pluginSpecFn, nullptr);
    if (!specObj) {
        Py_DECREF(processFn);
        Py_DECREF(pluginSpecFn);
        error = QStringLiteral("Plugin contract error: plugin_spec() raised: %1").arg(pyErrorToString());
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }
    if (!PyDict_Check(specObj)) {
        Py_DECREF(specObj);
        Py_DECREF(processFn);
        Py_DECREF(pluginSpecFn);
        error = QStringLiteral("Plugin contract error: plugin_spec() must return dict");
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    PyObject *apiVer = PyDict_GetItemString(specObj, "api_version");
    if (!apiVer || !PyUnicode_Check(apiVer)) {
        Py_DECREF(specObj);
        Py_DECREF(processFn);
        Py_DECREF(pluginSpecFn);
        error = QStringLiteral("Plugin contract error: missing string api_version in plugin_spec()");
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    QString apiVersion = QString::fromUtf8(PyUnicode_AsUTF8(apiVer));
    if (apiVersion != QStringLiteral("4b.image.v1")) {
        Py_DECREF(specObj);
        Py_DECREF(processFn);
        Py_DECREF(pluginSpecFn);
        error = QStringLiteral("Unsupported plugin api_version: %1").arg(apiVersion);
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    PyObject *nameObj = PyDict_GetItemString(specObj, "name");
    if (nameObj && PyUnicode_Check(nameObj))
        m_pluginName = QString::fromUtf8(PyUnicode_AsUTF8(nameObj));
    else
        m_pluginName = fi.baseName();

    PyObject *idObj = PyDict_GetItemString(specObj, "id");
    if (idObj && PyUnicode_Check(idObj))
        m_pluginId = QString::fromUtf8(PyUnicode_AsUTF8(idObj));
    else
        m_pluginId = fi.baseName();

    QVector<PythonPluginParamSpec> parsedSpecs;
    if (!parseParamSpecs(PyDict_GetItemString(specObj, "params"), parsedSpecs, error)) {
        Py_DECREF(specObj);
        Py_DECREF(processFn);
        Py_DECREF(pluginSpecFn);
        Py_DECREF(globals);
        Py_DECREF(locals);
        PyGILState_Release(gil);
        return false;
    }

    if (m_impl->module) {
        Py_DECREF(m_impl->module);
        m_impl->module = nullptr;
    }
    Py_INCREF(module);
    m_impl->module = module;
    m_pluginFile = fi.absoluteFilePath();
    m_paramSpecs = parsedSpecs;

    Py_DECREF(specObj);
    Py_DECREF(processFn);
    Py_DECREF(pluginSpecFn);
    Py_DECREF(globals);
    Py_DECREF(locals);
    PyGILState_Release(gil);

    return true;
#endif
}

QVariantMap PythonPluginRuntime::defaultParameters() const
{
    QVariantMap out;
    for (auto const &spec : m_paramSpecs)
        out.insert(spec.name, spec.defaultValue);
    return out;
}

std::optional<ImageData> PythonPluginRuntime::process(ImageData const &input, QString &error)
{
    return process(input, {}, error);
}

std::optional<ImageData> PythonPluginRuntime::process(ImageData const &input,
                                                      QVariantMap const &params,
                                                      QString &error)
{
    error.clear();

#ifndef NODDLE_WITH_PYTHON_PLUGIN
    Q_UNUSED(input)
    error = QStringLiteral("Python plugin support is disabled at build time");
    return std::nullopt;
#else
    if (!m_impl || !m_impl->module) {
        error = QStringLiteral("No plugin loaded");
        return std::nullopt;
    }
    QImage src = normalizeInputImage(input.image(), input.colorSpace());
    if (src.isNull()) {
        error = QStringLiteral("Invalid input image");
        return std::nullopt;
    }

    QMutexLocker lock(&pythonMutex());
    PyGILState_STATE gil = PyGILState_Ensure();

    PyObject *processFn = PyObject_GetAttrString(m_impl->module, "process");
    if (!processFn || !PyCallable_Check(processFn)) {
        Py_XDECREF(processFn);
        error = QStringLiteral("Plugin contract error: missing callable process(...)");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    PyObject *inputDict = PyDict_New();
    PyDict_SetItemString(inputDict, "width", PyLong_FromLong(src.width()));
    PyDict_SetItemString(inputDict, "height", PyLong_FromLong(src.height()));
    PyDict_SetItemString(inputDict, "channels", PyLong_FromLong(input.channels()));
    PyDict_SetItemString(inputDict, "dtype", PyUnicode_FromString("uint8"));
    PyDict_SetItemString(inputDict, "layout", PyUnicode_FromString("HWC"));
    PyDict_SetItemString(inputDict, "color_space", PyUnicode_FromString(colorSpaceString(input.colorSpace()).toUtf8().constData()));
    PyDict_SetItemString(inputDict, "row_stride", PyLong_FromLong(src.bytesPerLine()));

    QByteArray bytes(reinterpret_cast<char const *>(src.constBits()), src.sizeInBytes());
    PyObject *bytesObj = PyBytes_FromStringAndSize(bytes.constData(), bytes.size());
    PyDict_SetItemString(inputDict, "data", bytesObj);
    Py_DECREF(bytesObj);

    QVariantMap mergedParams = defaultParameters();
    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
        mergedParams.insert(it.key(), it.value());

    PyObject *paramsDict = PyDict_New();
    for (auto it = mergedParams.constBegin(); it != mergedParams.constEnd(); ++it) {
        PyObject *paramValue = variantToPyObject(it.value());
        PyDict_SetItemString(paramsDict, it.key().toUtf8().constData(), paramValue);
        Py_DECREF(paramValue);
    }
    PyObject *context = PyDict_New();
    PyDict_SetItemString(context, "frame_id", PyLong_FromLong(0));
    PyDict_SetItemString(context, "timestamp_ms", PyLong_FromLong(0));
    PyDict_SetItemString(context, "cancellation_requested", Py_False);

    PyObject *args = PyTuple_New(3);
    PyTuple_SetItem(args, 0, inputDict);
    PyTuple_SetItem(args, 1, paramsDict);
    PyTuple_SetItem(args, 2, context);

    PyObject *result = PyObject_CallObject(processFn, args);
    Py_DECREF(args);
    Py_DECREF(processFn);

    if (!result) {
        error = QStringLiteral("Plugin process error: %1").arg(pyErrorToString());
        PyGILState_Release(gil);
        return std::nullopt;
    }

    if (!PyDict_Check(result)) {
        Py_DECREF(result);
        error = QStringLiteral("Plugin process() must return dict");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    auto getLong = [result](char const *key, long def) {
        PyObject *v = PyDict_GetItemString(result, key);
        return v && PyLong_Check(v) ? PyLong_AsLong(v) : def;
    };

    int width = static_cast<int>(getLong("width", 0));
    int height = static_cast<int>(getLong("height", 0));
    int channels = static_cast<int>(getLong("channels", 0));
    int rowStride = static_cast<int>(getLong("row_stride", 0));

    PyObject *colorObj = PyDict_GetItemString(result, "color_space");
    QString colorStr = colorObj && PyUnicode_Check(colorObj)
        ? QString::fromUtf8(PyUnicode_AsUTF8(colorObj))
        : QStringLiteral("RGB");

    PyObject *dataObj = PyDict_GetItemString(result, "data");
    if (!dataObj || !PyBytes_Check(dataObj)) {
        Py_DECREF(result);
        error = QStringLiteral("Plugin output must contain bytes field data");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    char *raw = nullptr;
    Py_ssize_t len = 0;
    if (PyBytes_AsStringAndSize(dataObj, &raw, &len) != 0) {
        Py_DECREF(result);
        error = QStringLiteral("Unable to parse plugin output data bytes");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    if (width <= 0 || height <= 0 || (channels != 1 && channels != 3)) {
        Py_DECREF(result);
        error = QStringLiteral("Invalid plugin output dimensions/channels");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    if (rowStride <= 0)
        rowStride = width * channels;

    if (len < static_cast<Py_ssize_t>(height * rowStride)) {
        Py_DECREF(result);
        error = QStringLiteral("Plugin output data size is too small");
        PyGILState_Release(gil);
        return std::nullopt;
    }

    QImage out;
    if (channels == 1) {
        out = QImage(reinterpret_cast<uchar const *>(raw), width, height, rowStride,
                     QImage::Format_Grayscale8).copy();
    } else {
        out = QImage(reinterpret_cast<uchar const *>(raw), width, height, rowStride,
                     QImage::Format_RGB888).copy();
    }

    Py_DECREF(result);
    PyGILState_Release(gil);

    if (out.isNull()) {
        error = QStringLiteral("Failed to build output image from plugin result");
        return std::nullopt;
    }

    return ImageData(out, parseColorSpace(colorStr));
#endif
}

QVector<PluginDescriptor> PythonPluginRuntime::scanDirectory(QString const &dirPath)
{
    QVector<PluginDescriptor> result;

#ifdef NODDLE_WITH_PYTHON_PLUGIN
    QDir dir(dirPath);
    if (!dir.exists())
        return result;

    QStringList pyFiles = dir.entryList(QStringList() << QStringLiteral("*.py"), QDir::Files);
    for (auto const &fileName : pyFiles) {
        QString fullPath = dir.absoluteFilePath(fileName);
        PythonPluginRuntime probe;
        QString err;
        if (!probe.setPluginFile(fullPath, err))
            continue;
        PluginDescriptor desc;
        desc.id = probe.pluginId();
        desc.name = probe.pluginName();
        desc.filePath = fullPath;
        result.append(desc);
    }
#else
    Q_UNUSED(dirPath)
#endif

    return result;
}

} // namespace noddle
