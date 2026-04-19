#include "nodes/CameraSourceModel.hpp"

#include <QVideoFrame>

CameraSourceModel::CameraSourceModel()
{
    populateCameraList();

    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000);
    connect(m_fpsTimer, &QTimer::timeout, this, &CameraSourceModel::onUpdateFpsLabel);
}

CameraSourceModel::~CameraSourceModel()
{
    stopCamera();
}

unsigned int CameraSourceModel::nPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::Out)
        return 1;
    return 0;
}

QtNodes::NodeDataType CameraSourceModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return ImageData(QImage()).type();
}

std::shared_ptr<QtNodes::NodeData> CameraSourceModel::outData(QtNodes::PortIndex)
{
    return m_imageData;
}

QWidget *CameraSourceModel::embeddedWidget()
{
    if (!m_widget) {
        m_widget = new QWidget();
        auto *layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        m_cameraCombo = new QComboBox();
        for (auto const &cam : m_cameras)
            m_cameraCombo->addItem(cam.description());
        if (m_cameras.isEmpty())
            m_cameraCombo->addItem("No camera found");
        m_cameraCombo->setEnabled(!m_cameras.isEmpty());
        connect(m_cameraCombo, &QComboBox::currentIndexChanged,
                this, &CameraSourceModel::onCameraDeviceChanged);
        layout->addWidget(m_cameraCombo);

        m_preview = new QLabel("No feed");
        m_preview->setFixedSize(120, 90);
        m_preview->setAlignment(Qt::AlignCenter);
        m_preview->setStyleSheet("border: 1px solid #555; background: #222;");
        layout->addWidget(m_preview);

        m_toggleButton = new QPushButton("Start");
        m_toggleButton->setEnabled(!m_cameras.isEmpty());
        connect(m_toggleButton, &QPushButton::clicked,
                this, &CameraSourceModel::onToggleCamera);
        layout->addWidget(m_toggleButton);

        m_fpsLabel = new QLabel("FPS: —");
        m_fpsLabel->setStyleSheet("color: #aaa; font-size: 10px;");
        layout->addWidget(m_fpsLabel);
    }
    return m_widget;
}

void CameraSourceModel::onToggleCamera()
{
    if (m_running)
        stopCamera();
    else
        startCamera();
}

void CameraSourceModel::onCameraDeviceChanged(int index)
{
    if (m_running) {
        stopCamera();
        startCamera();
    }
    Q_UNUSED(index);
}

void CameraSourceModel::onVideoFrameChanged(const QVideoFrame &frame)
{
    // Frame rate limiting
    static constexpr int kMinFrameIntervalMs = 33; // ~30fps
    if (m_frameThrottle.isValid()
        && m_frameThrottle.elapsed() < kMinFrameIntervalMs) {
        return;
    }
    m_frameThrottle.restart();

    QVideoFrame mutableFrame(frame);
    mutableFrame.map(QVideoFrame::ReadOnly);
    QImage img = mutableFrame.toImage();
    mutableFrame.unmap();
    if (img.isNull())
        return;

    // Downscale early to reduce memory bandwidth in downstream nodes
    if (img.width() > 640) {
        img = img.scaled(640, 480, Qt::KeepAspectRatio, Qt::FastTransformation);
    }

    // Ensure usable format (avoid costly per-pixel conversion downstream)
    if (img.format() != QImage::Format_RGB32
        && img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_RGB32);
    }

    m_fpsCounter.tick();

    m_imageData = std::make_shared<ImageData>(img);

    if (m_preview) {
        m_preview->setPixmap(QPixmap::fromImage(
            img.scaled(120, 90, Qt::KeepAspectRatio, Qt::FastTransformation)));
    }

    Q_EMIT dataUpdated(0);
}

void CameraSourceModel::onUpdateFpsLabel()
{
    if (m_fpsLabel)
        m_fpsLabel->setText(QString("FPS: %1").arg(m_fpsCounter.fps(), 0, 'f', 1));
}

void CameraSourceModel::startCamera()
{
    int idx = m_cameraCombo ? m_cameraCombo->currentIndex() : 0;
    if (idx < 0 || idx >= m_cameras.size())
        return;

    m_captureSession = new QMediaCaptureSession(this);
    m_videoSink = new QVideoSink(this);
    m_captureSession->setVideoSink(m_videoSink);

    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &CameraSourceModel::onVideoFrameChanged);

    m_camera = new QCamera(m_cameras[idx], this);
    m_captureSession->setCamera(m_camera);

    m_camera->start();
    m_running = true;
    m_frameThrottle.start();
    m_fpsTimer->start();

    if (m_toggleButton)
        m_toggleButton->setText("Stop");
    if (m_cameraCombo)
        m_cameraCombo->setEnabled(false);
}

void CameraSourceModel::stopCamera()
{
    m_fpsTimer->stop();

    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
    delete m_captureSession;
    m_captureSession = nullptr;
    delete m_videoSink;
    m_videoSink = nullptr;

    m_running = false;

    if (m_toggleButton)
        m_toggleButton->setText("Start");
    if (m_cameraCombo)
        m_cameraCombo->setEnabled(!m_cameras.isEmpty());
    if (m_fpsLabel)
        m_fpsLabel->setText("FPS: —");
    if (m_preview)
        m_preview->setText("No feed");
}

void CameraSourceModel::populateCameraList()
{
    m_cameras = QMediaDevices::videoInputs();
}
