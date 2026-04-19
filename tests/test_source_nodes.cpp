#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cstring>
#include <fstream>
#include "noddle/core/source_nodes.hpp"

using namespace noddle::core;
using Catch::Approx;

namespace {

// Helper to write a temporary CSV file
std::string write_temp_csv(const std::string& name, const std::string& content) {
    std::string path = "/tmp/noddle_test_" + name;
    std::ofstream out(path);
    out << content;
    out.close();
    return path;
}

// Helper to read float values from a TensorPacket
std::vector<float> read_floats(const TensorPacket& p) {
    std::vector<float> result(p.tensor.element_count());
    std::memcpy(result.data(), p.tensor.bytes.data(), p.tensor.bytes.size());
    return result;
}

} // namespace

// ── ImageSourceNode ─────────────────────────────────────────────

TEST_CASE("ImageSourceNode — id returns source.image", "[source]") {
    ImageSourceNode node("dummy.png");
    REQUIRE(node.id() == "source.image");
}

TEST_CASE("ImageSourceNode — has_next initially true, false after consumption", "[source]") {
    ImageSourceNode node("dummy.png");
    REQUIRE(node.has_next());
    node.next();
    REQUIRE_FALSE(node.has_next());
}

TEST_CASE("ImageSourceNode — next returns valid placeholder tensor", "[source]") {
    ImageSourceNode node("dummy.png");
    auto packet = node.next();
    REQUIRE(packet.tensor.dtype == DType::Float32);
    REQUIRE(packet.tensor.shape == std::vector<std::size_t>{1});
    REQUIRE_NOTHROW(packet.validate());
}

TEST_CASE("ImageSourceNode — double consumption throws", "[source][edge]") {
    ImageSourceNode node("dummy.png");
    node.next();
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}

// ── CsvSourceNode ───────────────────────────────────────────────

TEST_CASE("CsvSourceNode — id returns source.csv", "[source]") {
    auto path = write_temp_csv("id.csv", "1,2\n3,4\n");
    CsvSourceNode node(path);
    REQUIRE(node.id() == "source.csv");
}

TEST_CASE("CsvSourceNode — reads 2x2 CSV correctly", "[source]") {
    auto path = write_temp_csv("2x2.csv", "1.0,2.0\n3.0,4.0\n");
    CsvSourceNode node(path);

    auto packet = node.next();
    REQUIRE(packet.tensor.shape == std::vector<std::size_t>{2, 2});
    REQUIRE(packet.tensor.dtype == DType::Float32);
    REQUIRE_NOTHROW(packet.validate());

    auto values = read_floats(packet);
    REQUIRE(values[0] == Approx(1.0f));
    REQUIRE(values[1] == Approx(2.0f));
    REQUIRE(values[2] == Approx(3.0f));
    REQUIRE(values[3] == Approx(4.0f));
}

TEST_CASE("CsvSourceNode — skips empty lines", "[source]") {
    auto path = write_temp_csv("empty_lines.csv", "1,2\n\n3,4\n");
    CsvSourceNode node(path);
    auto packet = node.next();
    REQUIRE(packet.tensor.shape == std::vector<std::size_t>{2, 2});
}

TEST_CASE("CsvSourceNode — throws on inconsistent columns", "[source][edge]") {
    auto path = write_temp_csv("bad_cols.csv", "1,2\n3,4,5\n");
    CsvSourceNode node(path);
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}

TEST_CASE("CsvSourceNode — throws on missing file", "[source][edge]") {
    CsvSourceNode node("/tmp/noddle_test_nonexistent_file_xyz.csv");
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}

TEST_CASE("CsvSourceNode — double consumption throws", "[source][edge]") {
    auto path = write_temp_csv("consumed.csv", "1,2\n");
    CsvSourceNode node(path);
    node.next();
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}

TEST_CASE("CsvSourceNode — custom separator", "[source]") {
    auto path = write_temp_csv("semicolon.csv", "1;2;3\n4;5;6\n");
    CsvSourceNode node(path, ';');
    auto packet = node.next();
    REQUIRE(packet.tensor.shape == std::vector<std::size_t>{2, 3});
}

// ── PointCloudSourceNode ────────────────────────────────────────

TEST_CASE("PointCloudSourceNode — id returns source.pointcloud", "[source]") {
    auto path = write_temp_csv("pc_id.csv", "1,2,3\n");
    PointCloudSourceNode node(path);
    REQUIRE(node.id() == "source.pointcloud");
}

TEST_CASE("PointCloudSourceNode — reads 3-column XYZ data", "[source]") {
    auto path = write_temp_csv("pc_xyz.csv", "1.0,2.0,3.0\n4.0,5.0,6.0\n");
    PointCloudSourceNode node(path);
    auto packet = node.next();

    REQUIRE(packet.tensor.shape == std::vector<std::size_t>{2, 3});
    REQUIRE_NOTHROW(packet.validate());

    auto values = read_floats(packet);
    REQUIRE(values[0] == Approx(1.0f));
    REQUIRE(values[5] == Approx(6.0f));
}

TEST_CASE("PointCloudSourceNode — throws on non-3 columns", "[source][edge]") {
    auto path = write_temp_csv("pc_bad.csv", "1,2\n3,4\n");
    PointCloudSourceNode node(path);
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}

TEST_CASE("PointCloudSourceNode — double consumption throws", "[source][edge]") {
    auto path = write_temp_csv("pc_consumed.csv", "1,2,3\n");
    PointCloudSourceNode node(path);
    node.next();
    REQUIRE_THROWS_AS(node.next(), std::runtime_error);
}
