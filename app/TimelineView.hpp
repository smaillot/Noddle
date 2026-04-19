#pragma once

#include <QDockWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QTimer>

#include "widgets/PipelineProfiler.hpp"

class TimelineWidget;
class StackedTimelineWidget;
class QStackedWidget;

class TimelineView : public QDockWidget
{
    Q_OBJECT

public:
    explicit TimelineView(QWidget *parent = nullptr);

private Q_SLOTS:
    void onRefresh();
    void onModeChanged(bool stacked);

private:
    TimelineWidget *m_timeline = nullptr;
    StackedTimelineWidget *m_stackedTimeline = nullptr;
    QStackedWidget *m_stack = nullptr;
    QComboBox *m_statCombo = nullptr;
    QComboBox *m_windowCombo = nullptr;
    QCheckBox *m_stackedCheck = nullptr;
    QTimer *m_refreshTimer = nullptr;
};
