#include <catch2/catch_test_macros.hpp>
#include "noddle/core/runtime.hpp"

using namespace noddle::core;

namespace {

// Minimal passthrough node for testing the execution backend
class PassthroughNode final : public INode {
public:
    std::string id() const override { return "test.passthrough"; }
    TensorPacket process(const TensorPacket& input) override { return input; }
};

// Node that doubles every UInt8 byte
class DoubleNode final : public INode {
public:
    std::string id() const override { return "test.double"; }
    TensorPacket process(const TensorPacket& input) override {
        TensorPacket result = input;
        for (auto& b : result.tensor.bytes) {
            b = static_cast<std::uint8_t>(b * 2);
        }
        return result;
    }
};

TensorPacket make_test_packet(std::vector<std::uint8_t> data, std::vector<std::size_t> shape) {
    TensorPacket p;
    p.tensor.dtype = DType::UInt8;
    p.tensor.shape = std::move(shape);
    p.tensor.bytes = std::move(data);
    p.sync_dimensions_from_tensor();
    return p;
}

} // namespace

// ── CpuBackend ──────────────────────────────────────────────────

TEST_CASE("CpuBackend — name returns cpu", "[runtime]") {
    auto backend = make_cpu_backend();
    REQUIRE(backend->name() == "cpu");
}

TEST_CASE("CpuBackend — run delegates to node::process", "[runtime]") {
    auto backend = make_cpu_backend();
    PassthroughNode node;
    auto input = make_test_packet({10, 20, 30}, {3});

    auto output = backend->run(node, input);
    REQUIRE(output.tensor.bytes == input.tensor.bytes);
    REQUIRE(output.tensor.shape == input.tensor.shape);
}

TEST_CASE("CpuBackend — run with processing node transforms data", "[runtime]") {
    auto backend = make_cpu_backend();
    DoubleNode node;
    auto input = make_test_packet({5, 10, 15}, {3});

    auto output = backend->run(node, input);
    REQUIRE(output.tensor.bytes == std::vector<std::uint8_t>{10, 20, 30});
}

TEST_CASE("CpuBackend — preserves metadata through execution", "[runtime]") {
    auto backend = make_cpu_backend();
    PassthroughNode node;
    auto input = make_test_packet({1, 2}, {2});

    auto output = backend->run(node, input);
    REQUIRE(output.metadata.dimensions == std::vector<std::size_t>{2});
}
