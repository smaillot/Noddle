#pragma once

#include <QImage>

#ifdef NODDLE_WITH_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

inline cv::Mat qImageToMat(QImage const &img)
{
    QImage converted = img.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(converted.height(), converted.width(), CV_8UC3,
                const_cast<uchar *>(converted.bits()),
                static_cast<size_t>(converted.bytesPerLine()));
    return mat.clone(); // deep copy — QImage owns the buffer
}

inline QImage matToQImage(cv::Mat const &mat)
{
    cv::Mat rgb;
    if (mat.channels() == 1)
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    else if (mat.channels() == 3)
        rgb = mat;
    else
        return QImage();

    return QImage(rgb.data, rgb.cols, rgb.rows,
                  static_cast<int>(rgb.step), QImage::Format_RGB888)
        .copy(); // deep copy — detach from cv::Mat memory
}
#endif
