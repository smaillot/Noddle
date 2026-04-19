#include <iostream>
#include <optional>

#include "noddle/core/runtime.hpp"
#include "noddle/core/source_nodes.hpp"
#include "noddle/core/visualization_policy.hpp"

namespace {

class EchoNode final : public noddle::core::INode {
public:
    std::string id() const override {
        return "echo";
    }

    noddle::core::TensorPacket process(const noddle::core::TensorPacket& input) override {
        return input;
    }
};

} // namespace

int main() {
    using namespace noddle::core;

    EchoNode node;
    auto backend = make_cpu_backend();

    TensorPacket packet;
    packet.tensor.dtype = DType::Float32;
    packet.tensor.shape = {1};
    packet.tensor.bytes.resize(sizeof(float));
    packet.sync_dimensions_from_tensor();

    auto output = backend->run(node, packet);

    VisualizationPolicy policy;
    auto stream_strategy = policy.resolve(FrameKind::Stream);
    auto single_strategy = policy.resolve(FrameKind::SingleFrame);
    auto forced_strategy = policy.resolve(
        FrameKind::Stream,
        std::optional<OverloadStrategy>(OverloadStrategy::Downsample)
    );

    std::cout << "Backend: " << backend->name() << "\n";
    std::cout << "Node: " << node.id() << "\n";
    std::cout << "Dims in metadata: " << output.metadata.dimensions.size() << "\n";
    std::cout << "Default stream strategy: " << static_cast<int>(stream_strategy) << "\n";
    std::cout << "Default single strategy: " << static_cast<int>(single_strategy) << "\n";
    std::cout << "Forced strategy: " << static_cast<int>(forced_strategy) << "\n";

    // Source nodes exist and are ready for concrete import adapters.
    ImageSourceNode image_source("example.png");
    CsvSourceNode csv_source("example.csv");
    PointCloudSourceNode cloud_source("points.csv");
    std::cout << image_source.id() << ", " << csv_source.id() << ", " << cloud_source.id() << "\n";
    return 0;
}
