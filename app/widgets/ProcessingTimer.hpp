#pragma once

#include <QElapsedTimer>

// Deprecated: use ScopeStageTimer (PipelineProfiler.hpp) for new code.
// Kept for reference; may be removed in a future cleanup.
class ProcessingTimer
{
public:
    void start() { m_timer.start(); }

    void stop()
    {
        qint64 ns = m_timer.nsecsElapsed();
        m_totalNs += ns;
        ++m_count;
        if (m_count >= 10) {
            m_avgMs = m_totalNs / (m_count * 1000000.0);
            m_totalNs = 0;
            m_count = 0;
        }
    }

    double avgMs() const { return m_avgMs; }

private:
    QElapsedTimer m_timer;
    qint64 m_totalNs = 0;
    int m_count = 0;
    double m_avgMs = 0.0;
};
