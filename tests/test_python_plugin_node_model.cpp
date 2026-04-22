#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "nodes/PythonPluginModel.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonObject>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

using Catch::Approx;

QApplication *ensureApp()
{
    if (qApp)
        return qobject_cast<QApplication *>(qApp);

    static int argc = 1;
    static char appName[] = "noddle_tests";
    static char *argv[] = {appName, nullptr};
    static QApplication app(argc, argv);
    return &app;
}

QString writePluginFile(QTemporaryDir &dir, QString const &name, QString const &content)
{
    QString path = dir.path() + QDir::separator() + name;
    QFile f(path);
    REQUIRE(f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream ts(&f);
    ts << content;
    f.close();
    return path;
}

} // namespace

TEST_CASE("PythonPluginRuntime — exposes typed parameter specs", "[plugin][python][runtime][params]")
{
    noddle::PythonPluginRuntime runtime;
    if (!runtime.isAvailable()) {
        SUCCEED("Python plugin support disabled in this build");
        return;
    }

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString pluginPath = writePluginFile(
        dir,
        "params_plugin.py",
        "def plugin_spec():\n"
        "    return {\n"
        "        'api_version': '4b.image.v1',\n"
        "        'id': 'demo.params',\n"
        "        'name': 'Params',\n"
        "        'params': {\n"
        "            'gain': {'type': 'float', 'default': 1.5, 'min': 0.0, 'max': 4.0, 'step': 0.25},\n"
        "            'enabled': {'type': 'bool', 'default': True},\n"
        "            'label': {'type': 'string', 'default': 'demo'},\n"
        "            'offset': {'type': 'int', 'default': 2, 'min': 0, 'max': 10}\n"
        "        }\n"
        "    }\n"
        "\n"
        "def process(input_image, params, context):\n"
        "    return input_image\n");

    QString err;
    REQUIRE(runtime.setPluginFile(pluginPath, err));

    auto specs = runtime.parameterSpecs();
    REQUIRE(specs.size() == 4);
    REQUIRE(specs[0].name == "gain");
    REQUIRE(specs[0].type == noddle::PythonPluginParamType::Double);
    REQUIRE(specs[0].defaultValue.toDouble() == 1.5);
    REQUIRE(specs[1].name == "enabled");
    REQUIRE(specs[1].type == noddle::PythonPluginParamType::Boolean);
    REQUIRE(specs[2].name == "label");
    REQUIRE(specs[2].type == noddle::PythonPluginParamType::String);
    REQUIRE(specs[3].name == "offset");
    REQUIRE(specs[3].type == noddle::PythonPluginParamType::Integer);
}

TEST_CASE("PythonPluginRuntime — passes parameter values to plugin process", "[plugin][python][runtime][params]")
{
    noddle::PythonPluginRuntime runtime;
    if (!runtime.isAvailable()) {
        SUCCEED("Python plugin support disabled in this build");
        return;
    }

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString pluginPath = writePluginFile(
        dir,
        "process_params_plugin.py",
        "def plugin_spec():\n"
        "    return {\n"
        "        'api_version': '4b.image.v1',\n"
        "        'id': 'demo.process_params',\n"
        "        'name': 'ProcessParams',\n"
        "        'params': {'delta': {'type': 'int', 'default': 0, 'min': 0, 'max': 255}}\n"
        "    }\n"
        "\n"
        "def process(input_image, params, context):\n"
        "    delta = int(params.get('delta', 0))\n"
        "    data = bytearray(input_image['data'])\n"
        "    data[0] = min(255, data[0] + delta)\n"
        "    return {\n"
        "        'width': input_image['width'],\n"
        "        'height': input_image['height'],\n"
        "        'channels': input_image['channels'],\n"
        "        'row_stride': input_image['row_stride'],\n"
        "        'color_space': input_image.get('color_space', 'RGB'),\n"
        "        'data': bytes(data),\n"
        "    }\n");

    QString err;
    REQUIRE(runtime.setPluginFile(pluginPath, err));

    QImage img(1, 1, QImage::Format_RGB888);
    img.setPixelColor(0, 0, QColor(10, 20, 30));

    QVariantMap params;
    params.insert("delta", 7);

    auto out = runtime.process(ImageData(img, ColorSpace::RGB), params, err);
    REQUIRE(out.has_value());
    REQUIRE(out->image().pixelColor(0, 0).red() == 17);
}

TEST_CASE("PythonPluginModel — saves and restores typed parameter values", "[plugin][python][node][params]")
{
    ensureApp();

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString pluginPath = writePluginFile(
        dir,
        "ui_params_plugin.py",
        "def plugin_spec():\n"
        "    return {\n"
        "        'api_version': '4b.image.v1',\n"
        "        'id': 'demo.ui_params',\n"
        "        'name': 'UiParams',\n"
        "        'params': {\n"
        "            'iterations': {'type': 'int', 'default': 2, 'min': 0, 'max': 10},\n"
        "            'mix': {'type': 'float', 'default': 0.5, 'min': 0.0, 'max': 1.0, 'step': 0.1},\n"
        "            'enabled': {'type': 'bool', 'default': True}\n"
        "        }\n"
        "    }\n"
        "\n"
        "def process(input_image, params, context):\n"
        "    return input_image\n");

    PythonPluginModel model;
    QJsonObject state;
    state["pluginPath"] = pluginPath;
    model.load(state);

    QWidget *widget = model.embeddedWidget();
    REQUIRE(widget != nullptr);

    auto *iterations = widget->findChild<QSpinBox *>("param_iterations");
    auto *mix = widget->findChild<QDoubleSpinBox *>("param_mix");
    auto *enabled = widget->findChild<QCheckBox *>("param_enabled");

    REQUIRE(iterations != nullptr);
    REQUIRE(mix != nullptr);
    REQUIRE(enabled != nullptr);

    iterations->setValue(6);
    mix->setValue(0.8);
    enabled->setChecked(false);

    QJsonObject saved = model.save();
    REQUIRE(saved.contains("params"));
    QJsonObject paramsJson = saved["params"].toObject();
    REQUIRE(paramsJson["iterations"].toInt() == 6);
    REQUIRE(paramsJson["mix"].toDouble() == Approx(0.8));
    REQUIRE(paramsJson["enabled"].toBool() == false);

    PythonPluginModel restored;
    restored.load(saved);
    QWidget *restoredWidget = restored.embeddedWidget();
    REQUIRE(restoredWidget != nullptr);

    auto *restoredIterations = restoredWidget->findChild<QSpinBox *>("param_iterations");
    auto *restoredMix = restoredWidget->findChild<QDoubleSpinBox *>("param_mix");
    auto *restoredEnabled = restoredWidget->findChild<QCheckBox *>("param_enabled");

    REQUIRE(restoredIterations->value() == 6);
    REQUIRE(restoredMix->value() == Approx(0.8));
    REQUIRE(restoredEnabled->isChecked() == false);
}
