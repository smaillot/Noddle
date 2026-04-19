#include "engine/PipelineExecutor.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtConcurrent>
#include <QFuture>

namespace noddle {

PipelineExecutor::PipelineExecutor(QtNodes::DataFlowGraphModel &model, QObject *parent)
    : QObject(parent)
    , m_model(model)
{
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

PipelineExecutor::~PipelineExecutor()
{
    if (m_running.load())
        stop();
}

void PipelineExecutor::rebuildSchedule()
{
    if (m_inFlight.load(std::memory_order_acquire)) {
        m_scheduleStale.store(true, std::memory_order_relaxed);
        return;
    }
    m_schedule = buildSchedule(m_model);
    cacheDelegates();
    m_scheduleStale.store(false, std::memory_order_relaxed);
}

void PipelineExecutor::cacheDelegates()
{
    m_delegates.clear();
    for (auto id : m_model.allNodeIds()) {
        auto *d = m_model.delegateModel<QtNodes::NodeDelegateModel>(id);
        if (d)
            m_delegates[id] = d;
    }
}

void PipelineExecutor::start()
{
    if (m_running.load())
        return;

    rebuildSchedule();
    if (!m_schedule.isValid())
        return;

    freezeAllNodes(true);

    // Connect to each source node's dataUpdated signal
    for (auto srcId : m_schedule.sourceNodes) {
        auto it = m_delegates.find(srcId);
        if (it == m_delegates.end())
            continue;

        m_dispatchers[srcId] = std::make_unique<FrameDispatcher>();

        auto conn = connect(it->second, &QtNodes::NodeDelegateModel::dataUpdated,
                            this, [this, srcId](QtNodes::PortIndex port) {
                                onSourceDataReady(srcId, port);
                            });
        m_sourceConnections.push_back(conn);
    }

    // Listen for topology changes to rebuild the schedule
    m_topologyConnections.push_back(
        connect(&m_model, &QtNodes::DataFlowGraphModel::connectionCreated,
                this, [this](QtNodes::ConnectionId const &) { rebuildSchedule(); }));
    m_topologyConnections.push_back(
        connect(&m_model, &QtNodes::DataFlowGraphModel::connectionDeleted,
                this, [this](QtNodes::ConnectionId const &) { rebuildSchedule(); }));
    m_topologyConnections.push_back(
        connect(&m_model, &QtNodes::DataFlowGraphModel::nodeCreated,
                this, [this](QtNodes::NodeId) { rebuildSchedule(); }));
    m_topologyConnections.push_back(
        connect(&m_model, &QtNodes::DataFlowGraphModel::nodeDeleted,
                this, [this](QtNodes::NodeId) { rebuildSchedule(); }));

    ++m_generation;
    m_running.store(true);
    m_paused.store(false);
    Q_EMIT started();
}

void PipelineExecutor::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    m_paused.store(false);

    // Increment generation so any queued UI callbacks become stale
    ++m_generation;

    // Wait for all in-flight work to finish
    m_threadPool.waitForDone();

    // Disconnect all source signals
    for (auto &conn : m_sourceConnections)
        QObject::disconnect(conn);
    m_sourceConnections.clear();

    // Disconnect topology signals
    for (auto &conn : m_topologyConnections)
        QObject::disconnect(conn);
    m_topologyConnections.clear();

    m_dispatchers.clear();
    m_delegates.clear();

    freezeAllNodes(false);

    Q_EMIT stopped();
}

void PipelineExecutor::setPaused(bool paused)
{
    m_paused.store(paused);
}

bool PipelineExecutor::isRunning() const { return m_running.load(); }
bool PipelineExecutor::isPaused() const { return m_paused.load(); }

void PipelineExecutor::setOverloadPolicy(QtNodes::NodeId sourceNodeId, OverloadPolicy policy)
{
    auto it = m_dispatchers.find(sourceNodeId);
    if (it != m_dispatchers.end())
        it->second->setPolicy(policy);
}

GraphSchedule const &PipelineExecutor::schedule() const { return m_schedule; }

void PipelineExecutor::onSourceDataReady(QtNodes::NodeId sourceNodeId,
                                         QtNodes::PortIndex /*port*/)
{
    if (!m_running.load() || m_paused.load())
        return;

    auto it = m_dispatchers.find(sourceNodeId);
    if (it == m_dispatchers.end())
        return;

    if (!it->second->acceptFrame())
        return;

    m_inFlight.store(true, std::memory_order_release);
    auto gen = m_generation;

    // Run the wavefront on the thread pool
    QtConcurrent::run(&m_threadPool, [this, sourceNodeId, gen]() {
        executeWavefront(sourceNodeId, gen);
    });
}

void PipelineExecutor::executeWavefront(QtNodes::NodeId sourceNodeId, uint64_t generation)
{
    // Execute levels 1..N (level 0 = sources, already produced data)
    for (std::size_t lvl = 1; lvl < m_schedule.levels.size(); ++lvl) {
        if (!m_running.load())
            break;

        auto const &level = m_schedule.levels[lvl];

        auto processNode = [this](QtNodes::NodeId nodeId) {
            auto dIt = m_delegates.find(nodeId);
            if (dIt == m_delegates.end())
                return;
            auto *delegate = dIt->second;

            // Use pre-built incoming edges — no DataFlowGraphModel access
            auto eIt = m_schedule.incomingEdges.find(nodeId);
            if (eIt == m_schedule.incomingEdges.end())
                return;

            for (auto const &edge : eIt->second) {
                auto uIt = m_delegates.find(edge.targetNode);
                if (uIt == m_delegates.end())
                    continue;
                delegate->setInData(uIt->second->outData(edge.outPort), edge.inPort);
            }
        };

        if (level.size() == 1) {
            processNode(level[0]);
        } else {
            QList<QFuture<void>> futures;
            for (auto nodeId : level) {
                futures.append(QtConcurrent::run(&m_threadPool, [processNode, nodeId]() {
                    processNode(nodeId);
                }));
            }
            for (auto &f : futures)
                f.waitForFinished();
        }
    }

    // Post UI refresh back to main thread
    QMetaObject::invokeMethod(
        this,
        [this, sourceNodeId, generation]() {
            // Stale wavefront — pipeline was stopped/restarted since dispatch
            if (generation != m_generation)
                return;

            for (std::size_t lvl = 0; lvl < m_schedule.levels.size(); ++lvl) {
                for (auto nodeId : m_schedule.levels[lvl]) {
                    refreshNodeWidgets(nodeId);
                    Q_EMIT nodeOutputReady(nodeId, 0);
                }
            }

            auto it = m_dispatchers.find(sourceNodeId);
            if (it != m_dispatchers.end())
                it->second->markCompleted();

            m_inFlight.store(false, std::memory_order_release);

            // Rebuild schedule if topology changed during wavefront
            if (m_scheduleStale.load(std::memory_order_relaxed))
                rebuildSchedule();

            Q_EMIT wavefrontCompleted();
        },
        Qt::QueuedConnection);
}

void PipelineExecutor::freezeAllNodes(bool frozen)
{
    for (auto id : m_model.allNodeIds()) {
        auto *delegate = m_model.delegateModel<QtNodes::NodeDelegateModel>(id);
        if (delegate)
            delegate->setFrozenState(frozen);
    }
}

void PipelineExecutor::refreshNodeWidgets(QtNodes::NodeId nodeId)
{
    auto it = m_delegates.find(nodeId);
    if (it == m_delegates.end())
        return;

    // Call refreshWidgets() if the node implements it (Q_INVOKABLE convention).
    // If the method doesn't exist, invokeMethod silently returns false.
    QMetaObject::invokeMethod(it->second, "refreshWidgets", Qt::DirectConnection);
}

} // namespace noddle
