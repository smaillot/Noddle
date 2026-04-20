// Integration tests for PipelineExecutor — async wavefront execution engine.
// Requires a QCoreApplication event loop (provided via custom main below).

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "engine/PipelineExecutor.hpp"
#include "engine/GraphSchedule.hpp"
#include "DummyModels.hpp"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <memory>
#include <set>

using QtNodes::ConnectionId;
using QtNodes::DataFlowGraphModel;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeId;

// ── Custom main — provides QCoreApplication for event loop ──────

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    return Catch::Session().run(argc, argv);
}

// ── Helpers ─────────────────────────────────────────────────────

namespace {

std::shared_ptr<NodeDelegateModelRegistry> makeIntegrationRegistry()
{
    auto reg = std::make_shared<NodeDelegateModelRegistry>();
    reg->registerModel<DummyDataSourceModel>("Test");
    reg->registerModel<DummyRecorderProcessModel>("Test");
    reg->registerModel<DummyRecorderModel>("Test");
    reg->registerModel<DummyProcessModel>("Test");
    reg->registerModel<DummySinkModel>("Test");
    return reg;
}

/// Helper: build A→B→C chain (source → process → sink).
struct ABCPipeline {
    std::shared_ptr<NodeDelegateModelRegistry> reg;
    std::unique_ptr<DataFlowGraphModel> model;
    NodeId a, b, c;

    ABCPipeline()
        : reg(makeIntegrationRegistry())
        , model(std::make_unique<DataFlowGraphModel>(reg))
    {
        a = model->addNode("DummyDataSource");
        b = model->addNode("DummyRecorderProcess");
        c = model->addNode("DummyRecorder");
        model->addConnection({a, 0, b, 0});
        model->addConnection({b, 0, c, 0});
    }
};

/// Helper: build A→B chain (source → recorder).
struct ABPipeline {
    std::shared_ptr<NodeDelegateModelRegistry> reg;
    std::unique_ptr<DataFlowGraphModel> model;
    NodeId a, b;

    ABPipeline()
        : reg(makeIntegrationRegistry())
        , model(std::make_unique<DataFlowGraphModel>(reg))
    {
        a = model->addNode("DummyDataSource");
        b = model->addNode("DummyRecorder");
        model->addConnection({a, 0, b, 0});
    }
};

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════
//  Integration tests — PipelineExecutor
// ═══════════════════════════════════════════════════════════════

TEST_CASE("PipelineExecutor — start/stop lifecycle",
          "[integration][engine]")
{
    ABCPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    QSignalSpy startedSpy(&executor, &noddle::PipelineExecutor::started);
    QSignalSpy stoppedSpy(&executor, &noddle::PipelineExecutor::stopped);

    REQUIRE_FALSE(executor.isRunning());

    executor.start();
    REQUIRE(executor.isRunning());
    REQUIRE(startedSpy.count() == 1);

    executor.stop();
    REQUIRE_FALSE(executor.isRunning());
    REQUIRE(stoppedSpy.count() == 1);
}

TEST_CASE("PipelineExecutor — nodes frozen on start, unfrozen on stop",
          "[integration][engine]")
{
    ABCPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    auto getDelegate = [&](NodeId id) {
        return p.model->delegateModel<QtNodes::NodeDelegateModel>(id);
    };

    // Not frozen initially
    REQUIRE_FALSE(getDelegate(p.a)->frozen());
    REQUIRE_FALSE(getDelegate(p.b)->frozen());
    REQUIRE_FALSE(getDelegate(p.c)->frozen());

    executor.start();
    REQUIRE(getDelegate(p.a)->frozen());
    REQUIRE(getDelegate(p.b)->frozen());
    REQUIRE(getDelegate(p.c)->frozen());

    executor.stop();
    REQUIRE_FALSE(getDelegate(p.a)->frozen());
    REQUIRE_FALSE(getDelegate(p.b)->frozen());
    REQUIRE_FALSE(getDelegate(p.c)->frozen());
}

TEST_CASE("PipelineExecutor — pause/unpause",
          "[integration][engine]")
{
    ABPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    executor.start();
    REQUIRE_FALSE(executor.isPaused());

    executor.setPaused(true);
    REQUIRE(executor.isPaused());

    executor.setPaused(false);
    REQUIRE_FALSE(executor.isPaused());

    executor.stop();
}

TEST_CASE("PipelineExecutor — start idempotent",
          "[integration][engine]")
{
    ABPipeline p;
    noddle::PipelineExecutor executor(*p.model);
    QSignalSpy startedSpy(&executor, &noddle::PipelineExecutor::started);

    executor.start();
    executor.start(); // second call should be a no-op
    REQUIRE(executor.isRunning());
    REQUIRE(startedSpy.count() == 1);

    executor.stop();
}

TEST_CASE("PipelineExecutor — stop idempotent",
          "[integration][engine]")
{
    ABPipeline p;
    noddle::PipelineExecutor executor(*p.model);
    QSignalSpy stoppedSpy(&executor, &noddle::PipelineExecutor::stopped);

    // Stop when not running — no-op, no crash, no signal
    executor.stop();
    REQUIRE_FALSE(executor.isRunning());
    REQUIRE(stoppedSpy.count() == 0);

    // Start then double-stop
    executor.start();
    executor.stop();
    executor.stop(); // second stop is no-op
    REQUIRE(stoppedSpy.count() == 1);
}

TEST_CASE("PipelineExecutor — schedule valid after start",
          "[integration][engine]")
{
    ABCPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    executor.start();

    auto const &sched = executor.schedule();
    REQUIRE(sched.isValid());
    // A→B→C should have 3 levels (source, process, sink)
    REQUIRE(sched.levels.size() == 3);
    REQUIRE(sched.sourceNodes.size() == 1);
    REQUIRE(sched.sourceNodes[0] == p.a);

    executor.stop();
}

TEST_CASE("PipelineExecutor — wavefront triggers on source data",
          "[integration][engine]")
{
    ABPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    // Get the recorder delegate
    auto *recorder = p.model->delegateModel<DummyRecorderModel>(p.b);
    REQUIRE(recorder != nullptr);

    // Get the source delegate
    auto *source = p.model->delegateModel<DummyDataSourceModel>(p.a);
    REQUIRE(source != nullptr);

    QSignalSpy wavefrontSpy(&executor, &noddle::PipelineExecutor::wavefrontCompleted);

    // Clear data recorded during graph construction (addConnection propagates nullptr)
    recorder->clearReceived();

    executor.start();

    // Emit data from source
    source->emitData();

    // Pump event loop until wavefront completes (max 2s)
    REQUIRE(wavefrontSpy.wait(2000));
    REQUIRE(wavefrontSpy.count() >= 1);

    // Recorder should have received non-null data from the wavefront
    auto received = recorder->received();
    REQUIRE(received.size() >= 1);
    REQUIRE(received[0].first != nullptr);

    executor.stop();
}

TEST_CASE("PipelineExecutor — nodeOutputReady emitted for all nodes",
          "[integration][engine]")
{
    ABCPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    auto *source = p.model->delegateModel<DummyDataSourceModel>(p.a);
    REQUIRE(source != nullptr);

    QSignalSpy outputSpy(&executor, &noddle::PipelineExecutor::nodeOutputReady);
    QSignalSpy wavefrontSpy(&executor, &noddle::PipelineExecutor::wavefrontCompleted);

    executor.start();

    source->emitData();

    // Wait for wavefront completion
    REQUIRE(wavefrontSpy.wait(2000));

    // Collect all NodeIds from nodeOutputReady signals
    std::set<NodeId> notified;
    for (auto const &args : outputSpy) {
        notified.insert(args.at(0).value<NodeId>());
    }

    // All three nodes should have been reported
    REQUIRE(notified.count(p.a) == 1);
    REQUIRE(notified.count(p.b) == 1);
    REQUIRE(notified.count(p.c) == 1);

    executor.stop();
}

TEST_CASE("PipelineExecutor — topology change rebuilds schedule",
          "[integration][engine]")
{
    auto reg = makeIntegrationRegistry();
    DataFlowGraphModel model(reg);

    // Start with A→B
    NodeId a = model.addNode("DummyDataSource");
    NodeId b = model.addNode("DummyRecorder");
    model.addConnection({a, 0, b, 0});

    noddle::PipelineExecutor executor(model);
    executor.start();

    auto const &sched1 = executor.schedule();
    REQUIRE(sched1.levels.size() == 2);

    // Add a node in between: A→C→B
    // First disconnect A→B, add C, connect A→C, connect C→B
    NodeId c = model.addNode("DummyRecorderProcess");

    // Process events to let topology signal trigger rebuildSchedule
    QCoreApplication::processEvents();

    auto const &sched2 = executor.schedule();
    // Schedule should have been rebuilt — now has 3 nodes
    // (the exact structure depends on connections, but schedule should have updated)
    REQUIRE(sched2.isValid());

    // The node count in all levels combined should include c
    std::set<NodeId> allNodes;
    for (auto const &level : sched2.levels)
        for (auto id : level)
            allNodes.insert(id);
    REQUIRE(allNodes.count(c) == 1);

    executor.stop();
}

TEST_CASE("PipelineExecutor — generation counter discards stale callbacks",
          "[integration][engine]")
{
    ABPipeline p;
    noddle::PipelineExecutor executor(*p.model);

    auto *source = p.model->delegateModel<DummyDataSourceModel>(p.a);
    REQUIRE(source != nullptr);

    executor.start();

    // Emit data then stop immediately — the stop increments generation,
    // so any queued UI callback from the wavefront becomes stale.
    source->emitData();
    executor.stop();

    // Pump remaining events — stale callbacks should be discarded, no crash
    QCoreApplication::processEvents();
    QTest::qWait(100);

    REQUIRE_FALSE(executor.isRunning());
}
