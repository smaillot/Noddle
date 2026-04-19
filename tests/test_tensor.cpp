#include <catch2/catch_test_macros.hpp>
#include "noddle/core/tensor.hpp"

using namespace noddle::core;

// ── DType ───────────────────────────────────────────────────────

TEST_CASE("dtype_size — returns correct byte sizes", "[tensor]") {
    REQUIRE(dtype_size(DType::UInt8) == 1);
    REQUIRE(dtype_size(DType::Float32) == 4);
    REQUIRE(dtype_size(DType::Float64) == 8);
}

// ── Tensor ──────────────────────────────────────────────────────

TEST_CASE("Tensor — element_count with empty shape returns 0", "[tensor][edge]") {
    Tensor t;
    t.shape = {};
    REQUIRE(t.element_count() == 0);
}

TEST_CASE("Tensor — element_count with scalar-like shape {1}", "[tensor]") {
    Tensor t;
    t.shape = {1};
    REQUIRE(t.element_count() == 1);
}

TEST_CASE("Tensor — element_count with multi-dimensional shape", "[tensor]") {
    Tensor t;
    t.shape = {3, 4, 5};
    REQUIRE(t.element_count() == 60);
}

TEST_CASE("Tensor — expected_nbytes matches dtype * element_count", "[tensor]") {
    Tensor t;
    t.dtype = DType::Float32;
    t.shape = {2, 3};
    REQUIRE(t.expected_nbytes() == 2 * 3 * sizeof(float));
}

TEST_CASE("Tensor — validate passes with correct byte size", "[tensor]") {
    Tensor t;
    t.dtype = DType::Float32;
    t.shape = {2};
    t.bytes.resize(2 * sizeof(float));
    REQUIRE_NOTHROW(t.validate());
}

TEST_CASE("Tensor — validate throws on byte size mismatch", "[tensor][edge]") {
    Tensor t;
    t.dtype = DType::Float32;
    t.shape = {2};
    t.bytes.resize(3); // wrong size
    REQUIRE_THROWS_AS(t.validate(), std::runtime_error);
}

TEST_CASE("Tensor — validate passes with empty shape and empty bytes", "[tensor][edge]") {
    Tensor t;
    t.shape = {};
    t.bytes = {};
    REQUIRE_NOTHROW(t.validate());
}

// ── TensorPacket ────────────────────────────────────────────────

TEST_CASE("TensorPacket — sync_dimensions_from_tensor copies shape", "[tensor]") {
    TensorPacket p;
    p.tensor.shape = {10, 20};
    p.sync_dimensions_from_tensor();
    REQUIRE(p.metadata.dimensions == std::vector<std::size_t>{10, 20});
}

TEST_CASE("TensorPacket — validate passes when metadata matches", "[tensor]") {
    TensorPacket p;
    p.tensor.dtype = DType::UInt8;
    p.tensor.shape = {4};
    p.tensor.bytes.resize(4);
    p.sync_dimensions_from_tensor();
    REQUIRE_NOTHROW(p.validate());
}

TEST_CASE("TensorPacket — validate throws on dimension mismatch", "[tensor][edge]") {
    TensorPacket p;
    p.tensor.dtype = DType::UInt8;
    p.tensor.shape = {4};
    p.tensor.bytes.resize(4);
    p.metadata.dimensions = {5}; // mismatched
    REQUIRE_THROWS_AS(p.validate(), std::runtime_error);
}

TEST_CASE("TensorPacket — validate throws on byte+shape mismatch", "[tensor][edge]") {
    TensorPacket p;
    p.tensor.dtype = DType::Float64;
    p.tensor.shape = {3};
    p.tensor.bytes.resize(1); // wrong
    p.sync_dimensions_from_tensor();
    REQUIRE_THROWS_AS(p.validate(), std::runtime_error);
}
