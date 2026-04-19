#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QCamera>
#include <QCameraDevice>
#include <QComboBox>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoSink>

#include "data/ImageData.hpp"
#include "widgets/FpsCounter.hpp"

class CameraSourceModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    CameraSourceModel();
    ~CameraSourceModel() override;

    QString name() const override { return QStringLiteral("CameraSource"); }
    QString caption() const override { return QStringLiteral("Camera Source"); }

    unsigned int nPorts(QtNodes::PortType portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override;

    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override;

    QWidget *embeddedWidget() override;

private Q_SLOTS:
    void onToggleCamera();
    void onCameraDeviceChanged(int index);
    void onVideoFrameChanged(const QVideoFrame &frame);
    void onUpdateFpsLabel();

private:
    void startCamera();
    void stopCamera();
    void populateCameraList();

    // Camera pipeline
    QCamera *m_camera = nullptr;
    QMediaCaptureSession *m_captureSession = nullptr;
    QVideoSink *m_videoSink = nullptr;

    // Frame rate limiting
    QElapsedTimer m_frameThrottle;

    // FPS tracking
    FpsCounter m_fpsCounter;
    QTimer *m_fpsTimer = nullptr;

    // Output data
    std::shared_ptr<ImageData> m_imageData;

    // Widget
    QWidget *m_widget = nullptr;
    QComboBox *m_cameraCombo = nullptr;
    QPushButton *m_toggleButton = nullptr;
    QLabel *m_preview = nullptr;
    QLabel *m_fpsLabel = nullptr;

    // Camera list
    QList<QCameraDevice> m_cameras;
    bool m_running = false;
};
