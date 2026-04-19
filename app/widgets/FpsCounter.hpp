#pragma once

#include <QElapsedTimer>

class FpsCounter
{
public:
    void tick()
    {
        if (!m_timer.isValid()) {
            m_timer.start();
            m_frameCount = 0;
            return;
        }

        ++m_frameCount;

        qint64 elapsed = m_timer.elapsed();
        if (elapsed >= 1000) {
            m_fps = m_frameCount * 1000.0 / elapsed;
            m_frameCount = 0;
            m_timer.restart();
        }
    }

    double fps() const { return m_fps; }

private:
    QElapsedTimer m_timer;
    int m_frameCount = 0;
    double m_fps = 0.0;
};
