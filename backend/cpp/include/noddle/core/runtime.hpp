#pragma once

#include <memory>
#include <string>

#include "noddle/core/tensor.hpp"

namespace noddle::core {

class INode {
public:
    virtual ~INode() = default;
    virtual std::string id() const = 0;
    virtual TensorPacket process(const TensorPacket& input) = 0;
};

class ISourceNode {
public:
    virtual ~ISourceNode() = default;
    virtual std::string id() const = 0;
    virtual bool has_next() const = 0;
    virtual TensorPacket next() = 0;
};

class IExecutionBackend {
public:
    virtual ~IExecutionBackend() = default;
    virtual std::string name() const = 0;
    virtual TensorPacket run(INode& node, const TensorPacket& input) = 0;
};

std::unique_ptr<IExecutionBackend> make_cpu_backend();

} // namespace noddle::core
