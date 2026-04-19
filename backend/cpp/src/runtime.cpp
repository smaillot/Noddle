#include "noddle/core/runtime.hpp"

namespace noddle::core {

class CpuBackend final : public IExecutionBackend {
public:
    std::string name() const override {
        return "cpu";
    }

    TensorPacket run(INode& node, const TensorPacket& input) override {
        return node.process(input);
    }
};

std::unique_ptr<IExecutionBackend> make_cpu_backend() {
    return std::make_unique<CpuBackend>();
}

} // namespace noddle::core
