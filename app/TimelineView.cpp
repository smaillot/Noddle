#include "TimelineView.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

#include <cmath>

// ---------- Ruler helpers ----------

static void drawDualRuler(QPainter &p, int areaLeft, int areaWidth,
                          double scaleMs, double scrollOffsetMs, int rulerHeight)
{
    p.fillRect(0, 0, areaLeft + areaWidth + 60, rulerHeight, QColor(25, 25, 25));

    QFont rulerFont;
    rulerFont.setPixelSize(9);
    p.setFont(rulerFont);

    // Compute "nice" tick spacing: pick the nearest value in
    // the 1-2-5 sequence so ruler labels don't overlap (min 60px apart).
    double pxPerMs = areaWidth / scaleMs;
    double minTickPx = 60.0;
    double rawTickMs = minTickPx / pxPerMs;
    double mag = std::pow(10.0, std::floor(std::log10(rawTickMs)));
    double norm = rawTickMs / mag;
    double niceNorm = (norm <= 1.0) ? 1.0 : (norm <= 2.0) ? 2.0 : (norm <= 5.0) ? 5.0 : 10.0;
    double tickMs = niceNorm * mag;
    if (tickMs < 0.01) tickMs = 0.01;

    double startMs = std::ceil(scrollOffsetMs / tickMs) * tickMs;

    for (double ms = startMs; ms < scrollOffsetMs + scaleMs; ms += tickMs) {
        int x = areaLeft + static_cast<int>((ms - scrollOffsetMs) * pxPerMs);
        if (x < areaLeft || x > areaLeft + areaWidth)
            continue;

        // ms label (top)
        p.setPen(QColor(160, 160, 160));
        p.drawLine(x, rulerHeight - 4, x, rulerHeight);
        QString msLabel = (tickMs >= 1.0) ? QString("%1").arg(ms, 0, 'f', 0)
                                          : QString("%1").arg(ms, 0, 'f', 2);
        p.drawText(x - 30, 0, 60, rulerHeight / 2, Qt::AlignCenter, msLabel + " ms");

        // fps label (bottom)
        if (ms > 0.001) {
            double fps = 1000.0 / ms;
            QString fpsLabel = QString("%1 fps").arg(fps, 0, 'f', (fps >= 10) ? 0 : 1);
            p.setPen(QColor(120, 120, 140));
            p.drawText(x - 30, rulerHeight / 2, 60, rulerHeight / 2, Qt::AlignCenter, fpsLabel);
        }
    }

    // Separator line
    p.setPen(QColor(60, 60, 60));
    p.drawLine(0, rulerHeight - 1, areaLeft + areaWidth + 60, rulerHeight - 1);
}

// ---------- TimelineWidget (bar chart mode) ----------

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumWidth(200);
    }

    void setStatType(PipelineProfiler::StatType t) { m_stat = t; }
    void setTimeWindow(PipelineProfiler::TimeWindow w) { m_window = w; }

    void refresh()
    {
        m_stages = PipelineProfiler::instance().snapshot(m_stat, m_window);
        m_totalMs = PipelineProfiler::instance().totalPipelineMs(m_stat, m_window);

        // Auto-scale only on first data
        if (!m_scaleInitialized && m_totalMs > 0.01) {
            m_scaleMs = m_totalMs * 1.3;
            m_scaleInitialized = true;
        }

        // Compute required height
        int rows = 0;
        QString lastNode;
        for (auto const &s : m_stages) {
            if (s.nodeCaption != lastNode) {
                ++rows;
                lastNode = s.nodeCaption;
            }
            ++rows;
        }
        rows += 2;
        int h = kRulerHeight + kTopMargin + rows * kRowHeight + kBottomMargin;
        setMinimumHeight(std::max(h, 120));
        update();
    }

    void setScaleMs(double ms) { m_scaleMs = ms; m_scaleInitialized = true; update(); }
    double scaleMs() const { return m_scaleMs; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.fillRect(rect(), QColor(30, 30, 30));

        int barAreaLeft = kLeftMargin + kLabelWidth + kGap;
        int barAreaWidth = width() - barAreaLeft - kRightMargin - kDurTextWidth;
        if (barAreaWidth < 40) barAreaWidth = 40;

        // Draw ruler
        drawDualRuler(p, barAreaLeft, barAreaWidth, m_scaleMs, 0.0, kRulerHeight);

        if (m_stages.isEmpty()) {
            p.setPen(QColor(140, 140, 140));
            QRect textR(0, kRulerHeight, width(), height() - kRulerHeight);
            p.drawText(textR, Qt::AlignCenter, "No profiling data");
            return;
        }

        int y = kRulerHeight + kTopMargin;

        static QColor const nodeColors[] = {
            QColor(80, 160, 220),  QColor(220, 140, 60),
            QColor(100, 200, 120), QColor(200, 100, 160),
            QColor(180, 180, 80),  QColor(140, 120, 220),
        };

        QString lastNode;
        int colorIdx = 0;

        QFont headerFont;
        headerFont.setPixelSize(12);
        headerFont.setBold(true);
        QFont stageFont;
        stageFont.setPixelSize(11);

        for (auto const &s : m_stages) {
            if (s.nodeCaption != lastNode) {
                lastNode = s.nodeCaption;
                p.setFont(headerFont);
                p.setPen(QColor(220, 220, 220));
                p.drawText(kLeftMargin, y, kLabelWidth, kRowHeight,
                           Qt::AlignLeft | Qt::AlignVCenter, s.nodeCaption);
                colorIdx = qHash(s.nodeCaption) % 6;
                y += kRowHeight;
            }

            QColor barColor = nodeColors[colorIdx];
            double ratio = (m_scaleMs > 0.0) ? s.durationMs / m_scaleMs : 0.0;
            int barW = static_cast<int>(ratio * barAreaWidth);
            if (barW < 2 && s.durationMs > 0) barW = 2;

            QRect barRect(barAreaLeft, y + 3, barW, kRowHeight - 6);
            p.setPen(Qt::NoPen);
            p.setBrush(barColor);
            p.drawRoundedRect(barRect, 3, 3);

            p.setFont(stageFont);
            p.setPen(QColor(180, 180, 180));
            p.drawText(kLeftMargin + 14, y, kLabelWidth - 18, kRowHeight,
                       Qt::AlignLeft | Qt::AlignVCenter, s.stageName);

            QString durText = QString("%1 ms").arg(s.durationMs, 0, 'f', 2);
            p.setPen(QColor(220, 220, 220));
            p.drawText(barAreaLeft + barAreaWidth + 4, y, kDurTextWidth, kRowHeight,
                       Qt::AlignLeft | Qt::AlignVCenter, durText);

            y += kRowHeight;
        }

        // Total + FPS
        y += 4;
        p.setPen(QColor(80, 80, 80));
        p.drawLine(kLeftMargin, y, width() - kRightMargin, y);
        y += 4;
        p.setFont(headerFont);
        p.setPen(QColor(220, 220, 220));
        double fps = (m_totalMs > 0.01) ? 1000.0 / m_totalMs : 0.0;
        QString totalText = QString("Total: %1 ms (%2 fps)")
                                .arg(m_totalMs, 0, 'f', 2)
                                .arg(fps, 0, 'f', 1);
        p.drawText(kLeftMargin, y, width() - kLeftMargin - kRightMargin, kRowHeight,
                   Qt::AlignLeft | Qt::AlignVCenter, totalText);
    }

    void wheelEvent(QWheelEvent *event) override
    {
        double factor = (event->angleDelta().y() > 0) ? 0.85 : 1.18;
        double newScale = m_scaleMs * factor;

        // Can't zoom out beyond total pipeline time (with margin)
        if (m_totalMs > 0.01) {
            double maxZoomOut = m_totalMs * 1.3;
            if (newScale > maxZoomOut)
                newScale = maxZoomOut;
        }
        m_scaleMs = std::max(0.1, newScale);
        m_scaleInitialized = true;
        update();
        event->accept();
    }

private:
    PipelineProfiler::StatType m_stat = PipelineProfiler::StatType::Avg;
    PipelineProfiler::TimeWindow m_window = PipelineProfiler::TimeWindow::Sec1;
    QVector<PipelineProfiler::StageSnapshot> m_stages;
    double m_totalMs = 0.0;
    double m_scaleMs = 50.0;
    bool m_scaleInitialized = false;

    static constexpr int kRowHeight = 22;
    static constexpr int kRulerHeight = 32;
    static constexpr int kTopMargin = 8;
    static constexpr int kBottomMargin = 20;
    static constexpr int kLeftMargin = 10;
    static constexpr int kRightMargin = 10;
    static constexpr int kLabelWidth = 140;
    static constexpr int kGap = 6;
    static constexpr int kDurTextWidth = 70;
};

// ---------- StackedTimelineWidget (single-line sequential frame view) ----------

class StackedTimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StackedTimelineWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setMinimumHeight(100);
    }

    void setStatType(PipelineProfiler::StatType s) { m_stat = s; }
    void setTimeWindow(PipelineProfiler::TimeWindow w) { m_window = w; }

    void refresh()
    {
        auto stages = PipelineProfiler::instance().snapshot(m_stat, m_window);
        m_totalMs = PipelineProfiler::instance().totalPipelineMs(m_stat, m_window);

        // Build a single frame from the snapshot (avg durations, t=0)
        m_stages = stages;

        // Compute maxFrameMs for zoom limit
        m_maxFrameMs = 0;
        for (auto const &s : m_stages)
            m_maxFrameMs += s.durationMs;

        // Auto-scale on first data
        if (!m_scaleInitialized && m_maxFrameMs > 0.01) {
            m_scaleMs = m_maxFrameMs * 1.2;
            m_scaleInitialized = true;
        }

        setMinimumHeight(kRulerHeight + kTopMargin + kFrameHeight + kBottomMargin + kTotalHeight + kLegendHeight);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.fillRect(rect(), QColor(30, 30, 30));

        int barAreaLeft = kLeftMargin;
        int barAreaWidth = width() - kLeftMargin - kRightMargin;
        if (barAreaWidth < 40) barAreaWidth = 40;

        // Ruler
        drawDualRuler(p, barAreaLeft, barAreaWidth, m_scaleMs, 0.0, kRulerHeight);

        if (m_stages.isEmpty()) {
            p.setPen(QColor(140, 140, 140));
            QRect textR(0, kRulerHeight, width(), height() - kRulerHeight);
            p.drawText(textR, Qt::AlignCenter, "No profiling data");
            return;
        }

        static QColor const nodeColors[] = {
            QColor(80, 160, 220),  QColor(220, 140, 60),
            QColor(100, 200, 120), QColor(200, 100, 160),
            QColor(180, 180, 80),  QColor(140, 120, 220),
        };

        double pxPerMs = barAreaWidth / m_scaleMs;

        // Draw stages side-by-side on a single line starting at t=0
        int y = kRulerHeight + kTopMargin;
        double offsetMs = 0.0;
        QFont eventFont;
        eventFont.setPixelSize(10);
        m_hitRects.clear();

        // Collect known nodes for legend
        QSet<QString> legendNodes;

        for (auto const &s : m_stages) {
            int x = barAreaLeft + static_cast<int>(offsetMs * pxPerMs);
            int w = std::max(2, static_cast<int>(s.durationMs * pxPerMs));

            int colorIdx = qHash(s.nodeCaption) % 6;
            QColor barColor = nodeColors[colorIdx];

            QRect barRect(x, y, w, kFrameHeight);
            p.setPen(Qt::NoPen);
            p.setBrush(barColor);
            p.drawRoundedRect(barRect, 2, 2);

            // Show stage name if bar is wide enough
            if (w > 40) {
                p.setFont(eventFont);
                p.setPen(QColor(255, 255, 255, 220));
                p.drawText(barRect.adjusted(4, 0, -2, 0),
                           Qt::AlignLeft | Qt::AlignVCenter, s.stageName);
            }

            PipelineProfiler::TimelineEvent ev;
            ev.nodeCaption = s.nodeCaption;
            ev.stageName = s.stageName;
            ev.timestampMs = 0;
            ev.durationMs = s.durationMs;
            m_hitRects.append({barRect, ev});
            legendNodes.insert(s.nodeCaption);
            offsetMs += s.durationMs;
        }

        // Total + FPS
        int totalY = y + kFrameHeight + 8;
        p.setPen(QColor(80, 80, 80));
        p.drawLine(kLeftMargin, totalY, width() - kRightMargin, totalY);
        totalY += 4;
        QFont headerFont;
        headerFont.setPixelSize(12);
        headerFont.setBold(true);
        p.setFont(headerFont);
        p.setPen(QColor(220, 220, 220));
        double fps = (m_totalMs > 0.01) ? 1000.0 / m_totalMs : 0.0;
        QString totalText = QString("Total: %1 ms (%2 fps)")
                                .arg(m_totalMs, 0, 'f', 2)
                                .arg(fps, 0, 'f', 1);
        p.drawText(kLeftMargin, totalY, width() - kLeftMargin - kRightMargin, 20,
                   Qt::AlignLeft | Qt::AlignVCenter, totalText);

        // Legend: colored squares with node names
        int legendY = totalY + 24;
        QFont legendFont;
        legendFont.setPixelSize(10);
        p.setFont(legendFont);
        int legendX = kLeftMargin;
        for (auto const &name : legendNodes) {
            int colorIdx = qHash(name) % 6;
            QColor c = nodeColors[colorIdx];
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(legendX, legendY + 2, 10, 10, 2, 2);
            p.setPen(QColor(180, 180, 180));
            QRect textR(legendX + 14, legendY, 200, 14);
            p.drawText(textR, Qt::AlignLeft | Qt::AlignVCenter, name);
            legendX += 14 + p.fontMetrics().horizontalAdvance(name) + 16;
        }
    }

    void wheelEvent(QWheelEvent *event) override
    {
        double factor = (event->angleDelta().y() > 0) ? 0.85 : 1.18;
        double newScale = m_scaleMs * factor;

        // Can't zoom out beyond max frame processing time (with margin)
        if (m_maxFrameMs > 0.01) {
            double maxZoomOut = m_maxFrameMs * 1.3;
            if (newScale > maxZoomOut)
                newScale = maxZoomOut;
        }
        m_scaleMs = std::max(0.1, newScale);
        m_scaleInitialized = true;

        update();
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        QPoint pos = event->pos();
        for (auto const &hr : m_hitRects) {
            if (hr.rect.contains(pos)) {
                QString tip = QString("%1 / %2\n%3 ms")
                                  .arg(hr.event.nodeCaption)
                                  .arg(hr.event.stageName)
                                  .arg(hr.event.durationMs, 0, 'f', 3);
                QToolTip::showText(event->globalPosition().toPoint(), tip, this);
                return;
            }
        }
        QToolTip::hideText();
    }

private:
    struct HitRect {
        QRect rect;
        PipelineProfiler::TimelineEvent event;
    };

    PipelineProfiler::StatType m_stat = PipelineProfiler::StatType::Avg;
    PipelineProfiler::TimeWindow m_window = PipelineProfiler::TimeWindow::Sec1;
    QVector<PipelineProfiler::StageSnapshot> m_stages;
    QVector<HitRect> m_hitRects;
    double m_totalMs = 0.0;
    double m_maxFrameMs = 0.0;
    double m_scaleMs = 50.0;
    bool m_scaleInitialized = false;

    static constexpr int kRulerHeight = 32;
    static constexpr int kTopMargin = 8;
    static constexpr int kBottomMargin = 8;
    static constexpr int kLeftMargin = 10;
    static constexpr int kRightMargin = 10;
    static constexpr int kFrameHeight = 28;
    static constexpr int kTotalHeight = 30;
    static constexpr int kLegendHeight = 24;
};

// ---------- TimelineView ----------

TimelineView::TimelineView(QWidget *parent)
    : QDockWidget(tr("Pipeline Timeline"), parent)
{
    setObjectName("TimelineView");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
                | QDockWidget::DockWidgetFloatable);

    auto *container = new QWidget();
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Stacked widget for two timeline modes
    m_stack = new QStackedWidget();

    // Mode 0: bar chart
    auto *scrollBar = new QScrollArea();
    scrollBar->setWidgetResizable(true);
    scrollBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_timeline = new TimelineWidget();
    scrollBar->setWidget(m_timeline);
    m_stack->addWidget(scrollBar);

    // Mode 1: stacked timeline
    auto *scrollStacked = new QScrollArea();
    scrollStacked->setWidgetResizable(true);
    scrollStacked->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_stackedTimeline = new StackedTimelineWidget();
    scrollStacked->setWidget(m_stackedTimeline);
    m_stack->addWidget(scrollStacked);

    mainLayout->addWidget(m_stack, 1);

    // Bottom controls
    auto *controlBar = new QWidget();
    controlBar->setObjectName("profilingControlBar");
    controlBar->setStyleSheet(
        "#profilingControlBar { background: #1e1e1e; }"
        "#profilingControlBar QLabel { color: #ddd; }"
        "#profilingControlBar QComboBox { color: #ddd; background: #3a3a3a; border: 1px solid #555; }"
        "#profilingControlBar QComboBox QAbstractItemView { background: #2d2d2d; color: #ddd; }"
        "#profilingControlBar QCheckBox { color: #ddd; }"
        "#profilingControlBar QCheckBox::indicator { border: 1px solid #888; background: #3a3a3a; width: 13px; height: 13px; }"
        "#profilingControlBar QCheckBox::indicator:checked { background: #5080c0; }"
    );
    auto *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(8, 4, 8, 4);

    controlLayout->addWidget(new QLabel("Stat:"));
    m_statCombo = new QComboBox();
    m_statCombo->addItem("Avg",    static_cast<int>(PipelineProfiler::StatType::Avg));
    m_statCombo->addItem("Min",    static_cast<int>(PipelineProfiler::StatType::Min));
    m_statCombo->addItem("Max",    static_cast<int>(PipelineProfiler::StatType::Max));
    m_statCombo->addItem("Median", static_cast<int>(PipelineProfiler::StatType::Median));
    controlLayout->addWidget(m_statCombo);

    controlLayout->addSpacing(12);

    controlLayout->addWidget(new QLabel("Window:"));
    m_windowCombo = new QComboBox();
    m_windowCombo->addItem("All time",    static_cast<int>(PipelineProfiler::TimeWindow::AllTime));
    m_windowCombo->addItem("Last frame",  static_cast<int>(PipelineProfiler::TimeWindow::LastFrame));
    m_windowCombo->addItem("10 frames",   static_cast<int>(PipelineProfiler::TimeWindow::Frames10));
    m_windowCombo->addItem("1 sec",       static_cast<int>(PipelineProfiler::TimeWindow::Sec1));
    m_windowCombo->addItem("5 sec",       static_cast<int>(PipelineProfiler::TimeWindow::Sec5));
    m_windowCombo->addItem("10 sec",      static_cast<int>(PipelineProfiler::TimeWindow::Sec10));
    m_windowCombo->setCurrentIndex(3); // default "1 sec"
    controlLayout->addWidget(m_windowCombo);

    controlLayout->addSpacing(16);

    m_stackedCheck = new QCheckBox("Stacked Timeline");
    controlLayout->addWidget(m_stackedCheck);

    controlLayout->addStretch();
    mainLayout->addWidget(controlBar);

    setWidget(container);

    // Auto-refresh at ~5 Hz
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(200);
    connect(m_refreshTimer, &QTimer::timeout, this, &TimelineView::onRefresh);
    m_refreshTimer->start();

    connect(m_statCombo, &QComboBox::currentIndexChanged, this, &TimelineView::onRefresh);
    connect(m_windowCombo, &QComboBox::currentIndexChanged, this, &TimelineView::onRefresh);
    connect(m_stackedCheck, &QCheckBox::toggled, this, &TimelineView::onModeChanged);
}

void TimelineView::onRefresh()
{
    auto stat = static_cast<PipelineProfiler::StatType>(
        m_statCombo->currentData().toInt());
    auto window = static_cast<PipelineProfiler::TimeWindow>(
        m_windowCombo->currentData().toInt());

    m_timeline->setStatType(stat);
    m_timeline->setTimeWindow(window);
    m_timeline->refresh();

    m_stackedTimeline->setStatType(stat);
    m_stackedTimeline->setTimeWindow(window);
    m_stackedTimeline->refresh();
}

void TimelineView::onModeChanged(bool stacked)
{
    m_stack->setCurrentIndex(stacked ? 1 : 0);
}

#include "TimelineView.moc"
