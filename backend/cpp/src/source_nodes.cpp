#include "noddle/core/source_nodes.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace noddle::core {

namespace {

std::vector<float> parse_csv_file(const std::string& path, char separator, std::size_t* rows, std::size_t* cols) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("failed to open csv file: " + path);
    }

    std::vector<float> values;
    std::size_t row_count = 0;
    std::size_t col_count = 0;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream line_stream(line);
        std::string cell;
        std::size_t current_cols = 0;

        while (std::getline(line_stream, cell, separator)) {
            values.push_back(std::stof(cell));
            ++current_cols;
        }

        if (col_count == 0) {
            col_count = current_cols;
        } else if (current_cols != col_count) {
            throw std::runtime_error("inconsistent csv column count in: " + path);
        }

        ++row_count;
    }

    *rows = row_count;
    *cols = col_count;
    return values;
}

TensorPacket make_float_packet(std::vector<float>&& values, const std::vector<std::size_t>& shape) {
    TensorPacket packet;
    packet.tensor.dtype = DType::Float32;
    packet.tensor.shape = shape;
    packet.tensor.bytes.resize(values.size() * sizeof(float));
    std::memcpy(packet.tensor.bytes.data(), values.data(), packet.tensor.bytes.size());
    packet.sync_dimensions_from_tensor();
    packet.validate();
    return packet;
}

} // namespace

ImageSourceNode::ImageSourceNode(std::string image_path)
    : image_path_(std::move(image_path)) {}

std::string ImageSourceNode::id() const {
    return "source.image";
}

bool ImageSourceNode::has_next() const {
    return !consumed_;
}

TensorPacket ImageSourceNode::next() {
    if (consumed_) {
        throw std::runtime_error("image source already consumed");
    }
    consumed_ = true;

    // Placeholder for MVP: import nodes normalize external formats into raw tensors.
    // Here we keep a minimal single-value tensor to validate the source pipeline path.
    return make_float_packet({0.0f}, {1});
}

CsvSourceNode::CsvSourceNode(std::string csv_path, char separator)
    : csv_path_(std::move(csv_path)), separator_(separator) {}

std::string CsvSourceNode::id() const {
    return "source.csv";
}

bool CsvSourceNode::has_next() const {
    return !consumed_;
}

TensorPacket CsvSourceNode::next() {
    if (consumed_) {
        throw std::runtime_error("csv source already consumed");
    }
    consumed_ = true;

    std::size_t rows = 0;
    std::size_t cols = 0;
    auto values = parse_csv_file(csv_path_, separator_, &rows, &cols);
    return make_float_packet(std::move(values), {rows, cols});
}

PointCloudSourceNode::PointCloudSourceNode(std::string csv_xyz_path)
    : csv_xyz_path_(std::move(csv_xyz_path)) {}

std::string PointCloudSourceNode::id() const {
    return "source.pointcloud";
}

bool PointCloudSourceNode::has_next() const {
    return !consumed_;
}

TensorPacket PointCloudSourceNode::next() {
    if (consumed_) {
        throw std::runtime_error("point cloud source already consumed");
    }
    consumed_ = true;

    std::size_t rows = 0;
    std::size_t cols = 0;
    auto values = parse_csv_file(csv_xyz_path_, ',', &rows, &cols);
    if (cols != 3) {
        throw std::runtime_error("point cloud csv must contain exactly 3 columns: x,y,z");
    }
    return make_float_packet(std::move(values), {rows, cols});
}

} // namespace noddle::core
