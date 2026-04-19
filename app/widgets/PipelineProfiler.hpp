#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>

#include <algorithm>

class PipelineProfiler : public QObject
{
    Q_OBJECT

public:
    static PipelineProfiler &instance()
    {
        static PipelineProfiler s;
        return s;
    }

    enum class StatType { Min, Max, Avg, Median };
    enum class TimeWindow { AllTime, LastFrame, Frames10, Sec1, Sec5, Sec10 };

    // Record a completed stage timing (ring buffer, max kMaxEntries per stage)
    void record(QString const &nodeCaption, QString const &stageName, double durationMs)
    {
        QMutexLocker lock(&m_mutex);
        if (!m_globalTimer.isValid())
            m_globalTimer.start();

        StageKey key{nodeCaption, stageName};
        auto &vec = m_entries[key];
        vec.append({m_globalTimer.elapsed(), durationMs, m_frameId});
        if (vec.size() > kMaxEntries)
            vec.remove(0, vec.size() - kMaxEntries);

        // Track stage ordering
        if (!m_stageOrder.contains(key))
            m_stageOrder.append(key);

        // Emit outside mutex to avoid deadlock with UI slot handlers
        lock.unlock();
        Q_EMIT updated();
    }

    // Mark a new frame boundary (call once per frame from source nodes)
    void markFrame(QString const &nodeCaption)
    {
        QMutexLocker lock(&m_mutex);
        if (!m_globalTimer.isValid())
            return; // No data recorded yet — nothing to mark
        ++m_frameId;
        m_fpsEntries[nodeCaption].append({m_globalTimer.elapsed(), m_frameId});
        auto &fv = m_fpsEntries[nodeCaption];
        if (fv.size() > kMaxEntries)
            fv.remove(0, fv.size() - kMaxEntries);
    }

    // Get stat for a specific stage
    double stat(QString const &nodeCaption, QString const &stageName,
                StatType type, TimeWindow window) const
    {
        QMutexLocker lock(&m_mutex);
        StageKey key{nodeCaption, stageName};
        auto it = m_entries.constFind(key);
        if (it == m_entries.constEnd())
            return 0.0;
        auto filtered = filterEntries(it.value(), window);
        return computeStat(filtered, type);
    }

    // FPS for a given node
    double fps(QString const &nodeCaption) const
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_fpsEntries.constFind(nodeCaption);
        if (it == m_fpsEntries.constEnd())
            return 0.0;
        auto const &fv = it.value();
        if (fv.size() < 2)
            return 0.0;
        // Use entries from last 1 second
        qint64 now = m_globalTimer.elapsed();
        int count = 0;
        qint64 earliest = now;
        for (int i = fv.size() - 1; i >= 0; --i) {
            if (now - fv[i].timestampMs > 1500)
                break;
            ++count;
            earliest = fv[i].timestampMs;
        }
        if (count < 2)
            return 0.0;
        double spanSec = (now - earliest) / 1000.0;
        if (spanSec < 0.001)
            return 0.0;
        return (count - 1) / spanSec;
    }

    // Total pipeline time for one frame
    double totalPipelineMs(StatType type, TimeWindow window) const
    {
        QMutexLocker lock(&m_mutex);
        // Sum all stage durations for each frame, then compute stat over frames
        if (m_entries.isEmpty())
            return 0.0;

        // For simplicity: sum stat values across all stages
        double total = 0.0;
        for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
            auto filtered = filterEntries(it.value(), window);
            total += computeStat(filtered, type);
        }
        return total;
    }

    struct StageSnapshot {
        QString nodeCaption;
        QString stageName;
        double durationMs;
    };

    // Chronological event for stacked timeline
    struct TimelineEvent {
        QString nodeCaption;
        QString stageName;
        qint64 timestampMs;   // ms since profiler start
        double durationMs;
    };

    // All known stages in order, with their stat values
    QVector<StageSnapshot> snapshot(StatType type, TimeWindow window) const
    {
        QMutexLocker lock(&m_mutex);
        QVector<StageSnapshot> result;
        for (auto const &key : m_stageOrder) {
            auto it = m_entries.constFind(key);
            if (it == m_entries.constEnd())
                continue;
            auto filtered = filterEntries(it.value(), window);
            double val = computeStat(filtered, type);
            result.append({key.first, key.second, val});
        }
        return result;
    }

    // Chronological events for stacked timeline (last windowMs of data)
    QVector<TimelineEvent> timelineEvents(qint64 windowMs) const
    {
        QMutexLocker lock(&m_mutex);
        QVector<TimelineEvent> result;
        qint64 now = m_globalTimer.isValid() ? m_globalTimer.elapsed() : 0;
        qint64 cutoff = now - windowMs;
        for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
            for (auto const &e : it.value()) {
                if (e.timestampMs >= cutoff) {
                    result.append({it.key().first, it.key().second,
                                   e.timestampMs, e.durationMs});
                }
            }
        }
        // Sort by timestamp
        std::sort(result.begin(), result.end(),
                  [](TimelineEvent const &a, TimelineEvent const &b) {
                      return a.timestampMs < b.timestampMs;
                  });
        return result;
    }

    // Reset all data
    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_entries.clear();
        m_fpsEntries.clear();
        m_stageOrder.clear();
        m_frameId = 0;
    }

Q_SIGNALS:
    void updated();

private:
    PipelineProfiler() = default;

    // Singleton — prevent copies/moves
    PipelineProfiler(PipelineProfiler const &) = delete;
    PipelineProfiler &operator=(PipelineProfiler const &) = delete;

    struct Entry {
        qint64 timestampMs;
        double durationMs;
        quint64 frameId;
    };

    struct FpsEntry {
        qint64 timestampMs;
        quint64 frameId;
    };

    using StageKey = QPair<QString, QString>;

    QVector<double> filterEntries(QVector<Entry> const &entries, TimeWindow window) const
    {
        QVector<double> result;
        if (entries.isEmpty())
            return result;

        switch (window) {
        case TimeWindow::LastFrame: {
            quint64 lastFrame = entries.last().frameId;
            for (auto const &e : entries) {
                if (e.frameId == lastFrame)
                    result.append(e.durationMs);
            }
            break;
        }
        case TimeWindow::Frames10: {
            quint64 lastFrame = entries.last().frameId;
            quint64 cutoff = lastFrame >= 10 ? lastFrame - 10 : 0;
            for (auto const &e : entries) {
                if (e.frameId > cutoff)
                    result.append(e.durationMs);
            }
            break;
        }
        case TimeWindow::Sec1:
        case TimeWindow::Sec5:
        case TimeWindow::Sec10: {
            qint64 now = m_globalTimer.elapsed();
            qint64 windowMs = (window == TimeWindow::Sec1)  ? 1000
                            : (window == TimeWindow::Sec5)  ? 5000
                                                            : 10000;
            for (auto const &e : entries) {
                if (now - e.timestampMs <= windowMs)
                    result.append(e.durationMs);
            }
            break;
        }
        case TimeWindow::AllTime:
        default:
            result.reserve(entries.size());
            for (auto const &e : entries)
                result.append(e.durationMs);
            break;
        }
        return result;
    }

    static double computeStat(QVector<double> const &values, StatType type)
    {
        if (values.isEmpty())
            return 0.0;

        switch (type) {
        case StatType::Min:
            return *std::min_element(values.begin(), values.end());
        case StatType::Max:
            return *std::max_element(values.begin(), values.end());
        case StatType::Avg: {
            double sum = 0.0;
            for (double v : values) sum += v;
            return sum / values.size();
        }
        case StatType::Median: {
            QVector<double> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            int n = sorted.size();
            if (n % 2 == 1)
                return sorted[n / 2];
            return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
        }
        }
        return 0.0;
    }

    mutable QMutex m_mutex;
    QElapsedTimer m_globalTimer;
    QMap<StageKey, QVector<Entry>> m_entries;
    QMap<QString, QVector<FpsEntry>> m_fpsEntries;
    QVector<StageKey> m_stageOrder;
    quint64 m_frameId = 0;

    static constexpr int kMaxEntries = 10000;
};

// RAII helper: records a stage timing on destruction
class ScopeStageTimer
{
public:
    ScopeStageTimer(QString nodeCaption, QString stageName)
        : m_nodeCaption(std::move(nodeCaption))
        , m_stageName(std::move(stageName))
    {
        m_timer.start();
    }

    ~ScopeStageTimer()
    {
        double ms = m_timer.nsecsElapsed() / 1.0e6;
        PipelineProfiler::instance().record(m_nodeCaption, m_stageName, ms);
    }

    ScopeStageTimer(ScopeStageTimer const &) = delete;
    ScopeStageTimer &operator=(ScopeStageTimer const &) = delete;

private:
    QString m_nodeCaption;
    QString m_stageName;
    QElapsedTimer m_timer;
};
