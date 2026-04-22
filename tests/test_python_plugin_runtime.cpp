#include <catch2/catch_test_macros.hpp>

#include "plugins/PythonPluginRuntime.hpp"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

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

QImage makeTestImage()
{
    QImage img(2, 2, QImage::Format_RGB888);
    img.fill(Qt::black);
    img.setPixelColor(0, 0, QColor(10, 20, 30));
    img.setPixelColor(1, 0, QColor(40, 50, 60));
    img.setPixelColor(0, 1, QColor(70, 80, 90));
    img.setPixelColor(1, 1, QColor(100, 110, 120));
    return img;
}

} // namespace

TEST_CASE("PythonPluginRuntime — unavailable build reports disabled", "[plugin][python][runtime]")
{
    noddle::PythonPluginRuntime runtime;

#ifndef NODDLE_WITH_PYTHON_PLUGIN
    REQUIRE_FALSE(runtime.isAvailable());
    QString err;
    REQUIRE_FALSE(runtime.setPluginFile("/tmp/does_not_matter.py", err));
    REQUIRE_FALSE(err.isEmpty());
#else
    REQUIRE(runtime.isAvailable());
#endif
}

TEST_CASE("PythonPluginRuntime — rejects missing plugin file", "[plugin][python][runtime][edge]")
{
    noddle::PythonPluginRuntime runtime;
    if (!runtime.isAvailable()) {
        SUCCEED("Python plugin support disabled in this build");
        return;
    }

    QString err;
    REQUIRE_FALSE(runtime.setPluginFile("/tmp/__noddle_missing_plugin__.py", err));
    REQUIRE(err.contains("not found", Qt::CaseInsensitive));
}

TEST_CASE("PythonPluginRuntime — loads valid plugin and processes image", "[plugin][python][runtime]")
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
        "identity_plugin.py",
        "def plugin_spec():\n"
        "    return {'api_version': '4b.image.v1', 'id': 'demo.identity', 'name': 'Identity'}\n"
        "\n"
        "def process(input_image, params, context):\n"
        "    return {\n"
        "        'width': input_image['width'],\n"
        "        'height': input_image['height'],\n"
        "        'channels': input_image['channels'],\n"
        "        'row_stride': input_image['row_stride'],\n"
        "        'color_space': input_image.get('color_space', 'RGB'),\n"
        "        'data': input_image['data'],\n"
        "    }\n");

    QString err;
    REQUIRE(runtime.setPluginFile(pluginPath, err));
    REQUIRE(err.isEmpty());

    ImageData input(makeTestImage(), ColorSpace::RGB);
    auto out = runtime.process(input, err);
    REQUIRE(out.has_value());
    REQUIRE(err.isEmpty());
    REQUIRE(out->image().size() == input.image().size());
    REQUIRE(out->channels() == input.channels());
}

TEST_CASE("PythonPluginRuntime — reports process exception", "[plugin][python][runtime][edge]")
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
        "error_plugin.py",
        "def plugin_spec():\n"
        "    return {'api_version': '4b.image.v1', 'id': 'demo.error', 'name': 'Error'}\n"
        "\n"
        "def process(input_image, params, context):\n"
        "    raise RuntimeError('boom from plugin')\n");

    QString err;
    REQUIRE(runtime.setPluginFile(pluginPath, err));

    ImageData input(makeTestImage(), ColorSpace::RGB);
    auto out = runtime.process(input, err);
    REQUIRE_FALSE(out.has_value());
    REQUIRE(err.contains("boom", Qt::CaseInsensitive));
}

// ──────────────────────────────────────────────
// scanDirectory tests
// ──────────────────────────────────────────────

TEST_CASE("PythonPluginRuntime — scanDirectory returns empty for missing folder", "[plugin][python][scan][edge]")
{
    auto result = noddle::PythonPluginRuntime::scanDirectory("/tmp/__noddle_nonexistent_dir_xyz__");
    REQUIRE(result.isEmpty());
}

TEST_CASE("PythonPluginRuntime — scanDirectory returns empty for empty folder", "[plugin][python][scan]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    auto result = noddle::PythonPluginRuntime::scanDirectory(dir.path());
    REQUIRE(result.isEmpty());
}

TEST_CASE("PythonPluginRuntime — scanDirectory finds valid plugins", "[plugin][python][scan]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    writePluginFile(dir, "plugin_a.py",
        "def plugin_spec():\n"
        "    return {'api_version': '4b.image.v1', 'id': 'test.alpha', 'name': 'Alpha'}\n"
        "def process(i, p, c): return i\n");

    writePluginFile(dir, "plugin_b.py",
        "def plugin_spec():\n"
        "    return {'api_version': '4b.image.v1', 'id': 'test.beta', 'name': 'Beta'}\n"
        "def process(i, p, c): return i\n");

    auto result = noddle::PythonPluginRuntime::scanDirectory(dir.path());
    REQUIRE(result.size() == 2);

    // Check IDs are present (order may vary)
    QStringList ids;
    for (auto const &d : result)
        ids << d.id;
    REQUIRE(ids.contains("test.alpha"));
    REQUIRE(ids.contains("test.beta"));

    // Check name is populated
    for (auto const &d : result)
        REQUIRE_FALSE(d.name.isEmpty());

    // Check filePath is valid
    for (auto const &d : result)
        REQUIRE(QFile::exists(d.filePath));
}

TEST_CASE("PythonPluginRuntime — scanDirectory ignores invalid plugin files", "[plugin][python][scan]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());

    // Valid plugin
    writePluginFile(dir, "valid.py",
        "def plugin_spec():\n"
        "    return {'api_version': '4b.image.v1', 'id': 'test.valid', 'name': 'Valid'}\n"
        "def process(i, p, c): return i\n");

    // Python file without plugin_spec — invalid
    writePluginFile(dir, "not_a_plugin.py",
        "x = 42\n");

    // Non-Python file — should be ignored
    writePluginFile(dir, "readme.txt",
        "This is not a plugin.\n");

    auto result = noddle::PythonPluginRuntime::scanDirectory(dir.path());
    REQUIRE(result.size() == 1);
    REQUIRE(result.front().id == "test.valid");
}
