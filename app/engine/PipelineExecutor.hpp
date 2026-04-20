#pragma once

#include <QObject>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/Definitions>
#include <QtNodes/NodeDelegateModel>

#include "engine/GraphSchedule.hpp"
#include "engine/FrameDispatcher.hpp"

namespace noddle {

class PipelineExecutor : public QObject
{
    Q_OBJECT

public:
    explicit PipelineExecutor(QtNodes::DataFlowGraphModel &model,
                              QObject *parent = nullptr);
    ~PipelineExecutor() override;

    void rebuildSchedule();
    void start();
    void stop();
    void setPaused(bool paused);

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isPaused() const;

    void setOverloadPolicy(QtNodes::NodeId sourceNodeId, OverloadPolicy policy);
    [[nodiscard]] GraphSchedule const &schedule() const;

Q_SIGNALS:
    void nodeOutputReady(QtNodes::NodeId nodeId, QtNodes::PortIndex portIndex);
    void wavefrontCompleted();
    void started();
    void stopped();

private:
    void onSourceDataReady(QtNodes::NodeId sourceNodeId, QtNodes::PortIndex portIndex);
    void executeWavefront(QtNodes::NodeId sourceNodeId, uint64_t generation);
    void freezeAllNodes(bool frozen);
    void refreshNodeWidgets(QtNodes::NodeId nodeId);
    void cacheDelegates();

    QtNodes::DataFlowGraphModel &m_model;
    GraphSchedule m_schedule;
    QThreadPool m_threadPool;

    std::unordered_map<QtNodes::NodeId, std::unique_ptr<FrameDispatcher>> m_dispatchers;
    std::unordered_map<QtNodes::NodeId, QtNodes::NodeDelegateModel *> m_delegates;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_inFlight{false};
    std::atomic<bool> m_scheduleStale{false};
    uint64_t m_generation{0};

    std::vector<QMetaObject::Connection> m_sourceConnections;
    std::vector<QMetaObject::Connection> m_topologyConnections;
};

} // namespace noddle
