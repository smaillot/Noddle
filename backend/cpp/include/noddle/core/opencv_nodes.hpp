#pragma once

#include <cstddef>
#include <string>

#include "noddle/core/runtime.hpp"

namespace noddle::core {

class ColorConvertNode final : public INode {
public:
    explicit ColorConvertNode(int color_code);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    int color_code_;
};

class ResizeNode final : public INode {
public:
    ResizeNode(int width, int height);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    int width_;
    int height_;
};

class GaussianBlurNode final : public INode {
public:
    explicit GaussianBlurNode(int kernel_size);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    int kernel_size_;
};

class ThresholdNode final : public INode {
public:
    ThresholdNode(double threshold, double max_value, int threshold_type);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    double threshold_;
    double max_value_;
    int threshold_type_;
};

class CannyNode final : public INode {
public:
    CannyNode(double threshold1, double threshold2);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    double threshold1_;
    double threshold2_;
};

class MorphologyNode final : public INode {
public:
    MorphologyNode(int op, int kernel_size);
    std::string id() const override;
    TensorPacket process(const TensorPacket& input) override;

private:
    int op_;
    int kernel_size_;
};

} // namespace noddle::core
