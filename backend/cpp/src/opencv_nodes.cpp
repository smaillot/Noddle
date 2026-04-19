#include "noddle/core/opencv_nodes.hpp"

#include <stdexcept>

#if defined(NODDLE_WITH_OPENCV)
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace noddle::core {

namespace {

#if defined(NODDLE_WITH_OPENCV)
cv::Mat tensor_to_mat(const TensorPacket& packet) {
    const auto& shape = packet.tensor.shape;
    if (packet.tensor.dtype != DType::UInt8) {
        throw std::runtime_error("opencv nodes currently require UInt8 tensors");
    }
    if (shape.size() == 2) {
        return cv::Mat(
            static_cast<int>(shape[0]),
            static_cast<int>(shape[1]),
            CV_8UC1,
            const_cast<std::uint8_t*>(packet.tensor.bytes.data())
        ).clone();
    }
    if (shape.size() == 3 && shape[2] == 3) {
        return cv::Mat(
            static_cast<int>(shape[0]),
            static_cast<int>(shape[1]),
            CV_8UC3,
            const_cast<std::uint8_t*>(packet.tensor.bytes.data())
        ).clone();
    }
    throw std::runtime_error("unsupported tensor shape for opencv mat conversion");
}

TensorPacket mat_to_tensor_packet(const cv::Mat& mat) {
    TensorPacket out;
    out.tensor.dtype = DType::UInt8;
    if (mat.channels() == 1) {
        out.tensor.shape = {
            static_cast<std::size_t>(mat.rows),
            static_cast<std::size_t>(mat.cols)
        };
    } else {
        out.tensor.shape = {
            static_cast<std::size_t>(mat.rows),
            static_cast<std::size_t>(mat.cols),
            static_cast<std::size_t>(mat.channels())
        };
    }
    out.tensor.bytes.assign(mat.datastart, mat.dataend);
    out.sync_dimensions_from_tensor();
    out.validate();
    return out;
}
#endif

TensorPacket opencv_unavailable() {
    throw std::runtime_error("opencv support is not enabled in this build");
}

} // namespace

ColorConvertNode::ColorConvertNode(int color_code)
    : color_code_(color_code) {}

std::string ColorConvertNode::id() const {
    return "opencv.cvt_color";
}

TensorPacket ColorConvertNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat dst;
    cv::cvtColor(src, dst, color_code_);
    return mat_to_tensor_packet(dst);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

ResizeNode::ResizeNode(int width, int height)
    : width_(width), height_(height) {}

std::string ResizeNode::id() const {
    return "opencv.resize";
}

TensorPacket ResizeNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat dst;
    cv::resize(src, dst, cv::Size(width_, height_));
    return mat_to_tensor_packet(dst);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

GaussianBlurNode::GaussianBlurNode(int kernel_size)
    : kernel_size_(kernel_size) {}

std::string GaussianBlurNode::id() const {
    return "opencv.gaussian_blur";
}

TensorPacket GaussianBlurNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat dst;
    cv::GaussianBlur(src, dst, cv::Size(kernel_size_, kernel_size_), 0.0);
    return mat_to_tensor_packet(dst);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

ThresholdNode::ThresholdNode(double threshold, double max_value, int threshold_type)
    : threshold_(threshold), max_value_(max_value), threshold_type_(threshold_type) {}

std::string ThresholdNode::id() const {
    return "opencv.threshold";
}

TensorPacket ThresholdNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src;
    }
    cv::Mat dst;
    cv::threshold(gray, dst, threshold_, max_value_, threshold_type_);
    return mat_to_tensor_packet(dst);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

CannyNode::CannyNode(double threshold1, double threshold2)
    : threshold1_(threshold1), threshold2_(threshold2) {}

std::string CannyNode::id() const {
    return "opencv.canny";
}

TensorPacket CannyNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src;
    }
    cv::Mat edges;
    cv::Canny(gray, edges, threshold1_, threshold2_);
    return mat_to_tensor_packet(edges);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

MorphologyNode::MorphologyNode(int op, int kernel_size)
    : op_(op), kernel_size_(kernel_size) {}

std::string MorphologyNode::id() const {
    return "opencv.morphology";
}

TensorPacket MorphologyNode::process(const TensorPacket& input) {
#if defined(NODDLE_WITH_OPENCV)
    cv::Mat src = tensor_to_mat(input);
    cv::Mat dst;
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(kernel_size_, kernel_size_)
    );
    cv::morphologyEx(src, dst, op_, kernel);
    return mat_to_tensor_packet(dst);
#else
    (void)input;
    return opencv_unavailable();
#endif
}

} // namespace noddle::core
