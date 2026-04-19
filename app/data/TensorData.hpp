#pragma once

#include <QtNodes/NodeData>
#include "noddle/core/tensor.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class TensorData : public NodeData
{
public:
    explicit TensorData(noddle::core::TensorPacket packet)
        : m_packet(std::move(packet))
    {}

    NodeDataType type() const override
    {
        return {"tensor", "Tensor"};
    }

    noddle::core::TensorPacket const &packet() const { return m_packet; }

private:
    noddle::core::TensorPacket m_packet;
};
