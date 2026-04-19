#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace noddle::core {

enum class DType {
    UInt8,
    Float32,
    Float64,
};

inline std::size_t dtype_size(DType dtype) {
    switch (dtype) {
        case DType::UInt8:
            return sizeof(std::uint8_t);
        case DType::Float32:
            return sizeof(float);
        case DType::Float64:
            return sizeof(double);
    }
    throw std::runtime_error("unsupported dtype");
}

struct Tensor {
    DType dtype {DType::Float32};
    std::vector<std::size_t> shape;
    std::vector<std::uint8_t> bytes;

    std::size_t element_count() const {
        if (shape.empty()) {
            return 0;
        }
        std::size_t count = 1;
        for (std::size_t dim : shape) {
            count *= dim;
        }
        return count;
    }

    std::size_t expected_nbytes() const {
        return element_count() * dtype_size(dtype);
    }

    void validate() const {
        if (bytes.size() != expected_nbytes()) {
            throw std::runtime_error("tensor byte size does not match dtype*shape");
        }
    }
};

struct TensorMetadata {
    std::vector<std::size_t> dimensions;
};

struct TensorPacket {
    Tensor tensor;
    TensorMetadata metadata;

    void sync_dimensions_from_tensor() {
        metadata.dimensions = tensor.shape;
    }

    void validate() const {
        tensor.validate();
        if (metadata.dimensions != tensor.shape) {
            throw std::runtime_error("metadata dimensions must match tensor shape");
        }
    }
};

} // namespace noddle::core
