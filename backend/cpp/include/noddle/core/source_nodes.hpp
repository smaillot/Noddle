#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "noddle/core/runtime.hpp"

namespace noddle::core {

class ImageSourceNode final : public ISourceNode {
public:
    explicit ImageSourceNode(std::string image_path);

    std::string id() const override;
    bool has_next() const override;
    TensorPacket next() override;

private:
    std::string image_path_;
    bool consumed_ {false};
};

class CsvSourceNode final : public ISourceNode {
public:
    CsvSourceNode(std::string csv_path, char separator = ',');

    std::string id() const override;
    bool has_next() const override;
    TensorPacket next() override;

private:
    std::string csv_path_;
    char separator_;
    bool consumed_ {false};
};

class PointCloudSourceNode final : public ISourceNode {
public:
    explicit PointCloudSourceNode(std::string csv_xyz_path);

    std::string id() const override;
    bool has_next() const override;
    TensorPacket next() override;

private:
    std::string csv_xyz_path_;
    bool consumed_ {false};
};

} // namespace noddle::core
