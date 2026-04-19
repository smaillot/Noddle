#include "nodes/CameraSourceModel.hpp"

#include <QVideoFrame>
#include <QtConcurrent/QtConcurrent>

#include "widgets/PipelineProfiler.hpp"

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

        m_formatCombo = new QComboBox();
        m_formatCombo->setEnabled(!m_cameras.isEmpty());
        m_formatCombo->setStyleSheet("font-size: 10px;");
        layout->addWidget(m_formatCombo);
        populateFormatList();

        m_preview = new QLabel("No feed");
        m_preview->setObjectName("nodePreview");
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
    Q_UNUSED(index);
    populateFormatList();
    if (m_running) {
        stopCamera();
        startCamera();
    }
}

void CameraSourceModel::onFormatChanged(int index)
{
    Q_UNUSED(index);
    if (m_running) {
        stopCamera();
        startCamera();
    }
}

void CameraSourceModel::onVideoFrameChanged(const QVideoFrame &frame)
{
    // Skip if still processing previous frame
    if (m_processing.exchange(true))
        return;

    QVideoFrame mutableFrame(frame);

    // Offload heavy GPU->CPU readback + format conversion to thread pool
    m_processingFuture = QtConcurrent::run([this, mutableFrame]() mutable {
        QElapsedTimer decodeTimer;
        decodeTimer.start();

        if (!mutableFrame.map(QVideoFrame::ReadOnly)) {
            m_processing.store(false);
            return;
        }
        QImage img = mutableFrame.toImage();
        mutableFrame.unmap();

        double decodeMs = decodeTimer.nsecsElapsed() / 1.0e6;
        PipelineProfiler::instance().record("Camera Source", "decode", decodeMs);

        if (img.isNull()) {
            m_processing.store(false);
            return;
        }

        // Ensure usable format
        QElapsedTimer convertTimer;
        convertTimer.start();
        if (img.format() != QImage::Format_RGB32
            && img.format() != QImage::Format_ARGB32) {
            img = img.convertToFormat(QImage::Format_RGB32);
        }
        double convertMs = convertTimer.nsecsElapsed() / 1.0e6;
        PipelineProfiler::instance().record("Camera Source", "convert", convertMs);

        // Release the frame-skip gate BEFORE posting to UI thread.
        // This decouples processing throughput from UI update latency:
        // the next camera frame can begin processing while the UI
        // thread is still rendering the previous result.
        m_processing.store(false);

        // Post result back to UI thread
        QMetaObject::invokeMethod(this, [this, img = std::move(img)]() {
            m_lastResolution = img.size();
            m_fpsCounter.tick();

            PipelineProfiler::instance().markFrame("Camera Source");

            m_imageData = std::make_shared<ImageData>(img);

            {
                ScopeStageTimer t("Camera Source", "preview");
                if (m_preview) {
                    m_preview->setPixmap(QPixmap::fromImage(
                        img.scaled(120, 90, Qt::KeepAspectRatio, Qt::FastTransformation)));
                }
            }

            Q_EMIT dataUpdated(0);
        });
    });
}

void CameraSourceModel::onUpdateFpsLabel()
{
    if (m_fpsLabel) {
        if (m_lastResolution.isValid()) {
            m_fpsLabel->setText(QString("%1x%2 @ %3 fps")
                                    .arg(m_lastResolution.width())
                                    .arg(m_lastResolution.height())
                                    .arg(m_fpsCounter.fps(), 0, 'f', 1));
        } else {
            m_fpsLabel->setText(QString("FPS: %1").arg(m_fpsCounter.fps(), 0, 'f', 1));
        }
    }
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

    // Apply selected format (resolution + FPS)
    int fmtIdx = m_formatCombo ? m_formatCombo->currentIndex() : -1;
    if (fmtIdx >= 0 && fmtIdx < m_formats.size()) {
        m_camera->setCameraFormat(m_formats[fmtIdx]);
    }

    m_camera->start();
    m_running = true;
    m_fpsTimer->start();

    if (m_toggleButton)
        m_toggleButton->setText("Stop");
    if (m_cameraCombo)
        m_cameraCombo->setEnabled(false);
    if (m_formatCombo)
        m_formatCombo->setEnabled(false);
}

void CameraSourceModel::stopCamera()
{
    m_fpsTimer->stop();

    // CRITICAL: wait for in-flight thread pool task before destroying
    // camera objects — prevents use-after-free on captured `this`
    if (m_processingFuture.isRunning())
        m_processingFuture.waitForFinished();

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
    if (m_formatCombo)
        m_formatCombo->setEnabled(!m_cameras.isEmpty());
    if (m_fpsLabel)
        m_fpsLabel->setText("FPS: —");
    if (m_preview)
        m_preview->setText("No feed");
}

void CameraSourceModel::populateCameraList()
{
    m_cameras = QMediaDevices::videoInputs();
}

void CameraSourceModel::populateFormatList()
{
    if (!m_formatCombo)
        return;

    m_formatCombo->blockSignals(true);
    m_formatCombo->clear();
    m_formats.clear();

    int idx = m_cameraCombo ? m_cameraCombo->currentIndex() : 0;
    if (idx < 0 || idx >= m_cameras.size()) {
        m_formatCombo->addItem("—");
        m_formatCombo->blockSignals(false);
        return;
    }

    auto const &camFormats = m_cameras[idx].videoFormats();
    int bestIdx = 0;
    for (int i = 0; i < camFormats.size(); ++i) {
        auto const &f = camFormats[i];
        auto res = f.resolution();
        QString label = QString("%1x%2 @ %3 fps")
                            .arg(res.width())
                            .arg(res.height())
                            .arg(f.maxFrameRate(), 0, 'f', 0);
        m_formatCombo->addItem(label);
        m_formats.append(f);

        // Prefer 640x480 @ highest fps as default
        if (res.width() == 640 && res.height() == 480)
            bestIdx = i;
    }

    if (m_formats.isEmpty())
        m_formatCombo->addItem("—");
    else
        m_formatCombo->setCurrentIndex(bestIdx);

    m_formatCombo->blockSignals(false);
    connect(m_formatCombo, &QComboBox::currentIndexChanged,
            this, &CameraSourceModel::onFormatChanged,
            Qt::UniqueConnection);
}
