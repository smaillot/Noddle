#include <catch2/catch_test_macros.hpp>

#include "engine/GraphSchedule.hpp"
#include "engine/FrameDispatcher.hpp"

#include "DummyModels.hpp"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

using QtNodes::ConnectionId;
using QtNodes::DataFlowGraphModel;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeId;

// ── Helpers ─────────────────────────────────────────────────────

namespace {

/// Build a shared registry with all dummy model types.
std::shared_ptr<NodeDelegateModelRegistry> makeTestRegistry()
{
    auto reg = std::make_shared<NodeDelegateModelRegistry>();
    reg->registerModel<DummySourceModel>("Test");
    reg->registerModel<DummyProcessModel>("Test");
    reg->registerModel<DummyMergeModel>("Test");
    reg->registerModel<DummySinkModel>("Test");
    return reg;
}

/// Collect NodeIds from an ExecutionLevel into a sorted vector for comparison.
std::vector<NodeId> sorted(noddle::ExecutionLevel const &level)
{
    std::vector<NodeId> v(level.begin(), level.end());
    std::sort(v.begin(), v.end());
    return v;
}

/// Collect NodeIds into a set for order-independent comparison.
std::set<NodeId> toSet(std::vector<NodeId> const &v)
{
    return {v.begin(), v.end()};
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════
//  GraphSchedule tests
// ═══════════════════════════════════════════════════════════════

TEST_CASE("GraphSchedule — empty graph produces invalid schedule",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    auto schedule = noddle::buildSchedule(model);

    REQUIRE_FALSE(schedule.isValid());
    REQUIRE(schedule.levels.empty());
    REQUIRE(schedule.sourceNodes.empty());
}

TEST_CASE("GraphSchedule — single source node gives 1 level, 1 source",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId s = model.addNode("DummySource");

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 1);
    REQUIRE(schedule.levels[0].size() == 1);
    REQUIRE(schedule.levels[0][0] == s);
    REQUIRE(schedule.sourceNodes.size() == 1);
    REQUIRE(schedule.sourceNodes[0] == s);
    REQUIRE(schedule.nodeLevels.at(s) == 0);
}

TEST_CASE("GraphSchedule — linear chain A→B→C gives 3 levels",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId a = model.addNode("DummySource");
    NodeId b = model.addNode("DummyProcess");
    NodeId c = model.addNode("DummyProcess");

    model.addConnection(ConnectionId{a, 0, b, 0});
    model.addConnection(ConnectionId{b, 0, c, 0});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 3);

    // A is at level 0
    REQUIRE(schedule.nodeLevels.at(a) == 0);
    // B is at level 1
    REQUIRE(schedule.nodeLevels.at(b) == 1);
    // C is at level 2
    REQUIRE(schedule.nodeLevels.at(c) == 2);

    // Only A is a source
    REQUIRE(schedule.sourceNodes.size() == 1);
    REQUIRE(schedule.sourceNodes[0] == a);
}

TEST_CASE("GraphSchedule — diamond A→B, A→C, B→D, C→D gives 3 levels with B,C parallelizable",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId a = model.addNode("DummySource");
    NodeId b = model.addNode("DummyProcess");
    NodeId c = model.addNode("DummyProcess");
    NodeId d = model.addNode("DummyMerge");

    model.addConnection(ConnectionId{a, 0, b, 0});
    model.addConnection(ConnectionId{a, 0, c, 0});
    model.addConnection(ConnectionId{b, 0, d, 0});
    model.addConnection(ConnectionId{c, 0, d, 1});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 3);

    // A at level 0
    REQUIRE(schedule.nodeLevels.at(a) == 0);

    // B and C at level 1 (parallelizable)
    REQUIRE(schedule.nodeLevels.at(b) == 1);
    REQUIRE(schedule.nodeLevels.at(c) == 1);
    REQUIRE(sorted(schedule.levels[1]).size() == 2);

    auto level1Set = toSet(schedule.levels[1]);
    REQUIRE(level1Set.count(b));
    REQUIRE(level1Set.count(c));

    // D at level 2
    REQUIRE(schedule.nodeLevels.at(d) == 2);
}

TEST_CASE("GraphSchedule — multi-source S1→A, S2→A gives 2 levels, 2 sources",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId s1 = model.addNode("DummySource");
    NodeId s2 = model.addNode("DummySource");
    NodeId a  = model.addNode("DummyMerge");

    model.addConnection(ConnectionId{s1, 0, a, 0});
    model.addConnection(ConnectionId{s2, 0, a, 1});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 2);

    // Both sources at level 0
    REQUIRE(schedule.nodeLevels.at(s1) == 0);
    REQUIRE(schedule.nodeLevels.at(s2) == 0);

    auto srcSet = toSet(schedule.sourceNodes);
    REQUIRE(srcSet.size() == 2);
    REQUIRE(srcSet.count(s1));
    REQUIRE(srcSet.count(s2));

    // A at level 1
    REQUIRE(schedule.nodeLevels.at(a) == 1);
}

TEST_CASE("GraphSchedule — disconnected islands {A→B} {C→D} gives 2 sources, 2 levels",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    // Island 1
    NodeId a = model.addNode("DummySource");
    NodeId b = model.addNode("DummyProcess");
    model.addConnection(ConnectionId{a, 0, b, 0});

    // Island 2
    NodeId c = model.addNode("DummySource");
    NodeId d = model.addNode("DummyProcess");
    model.addConnection(ConnectionId{c, 0, d, 0});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 2);

    // Both A and C are sources
    auto srcSet = toSet(schedule.sourceNodes);
    REQUIRE(srcSet.size() == 2);
    REQUIRE(srcSet.count(a));
    REQUIRE(srcSet.count(c));

    // A, C at level 0; B, D at level 1
    REQUIRE(schedule.nodeLevels.at(a) == 0);
    REQUIRE(schedule.nodeLevels.at(c) == 0);
    REQUIRE(schedule.nodeLevels.at(b) == 1);
    REQUIRE(schedule.nodeLevels.at(d) == 1);
}

TEST_CASE("GraphSchedule — fan-out A→B, A→C, A→D gives 2 levels, 3 at level 1",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId a = model.addNode("DummySource");
    NodeId b = model.addNode("DummyProcess");
    NodeId c = model.addNode("DummyProcess");
    NodeId d = model.addNode("DummyProcess");

    model.addConnection(ConnectionId{a, 0, b, 0});
    model.addConnection(ConnectionId{a, 0, c, 0});
    model.addConnection(ConnectionId{a, 0, d, 0});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 2);

    // Level 0: only A
    REQUIRE(schedule.levels[0].size() == 1);
    REQUIRE(schedule.levels[0][0] == a);

    // Level 1: B, C, D (order may vary)
    REQUIRE(schedule.levels[1].size() == 3);
    auto level1Set = toSet(schedule.levels[1]);
    REQUIRE(level1Set.count(b));
    REQUIRE(level1Set.count(c));
    REQUIRE(level1Set.count(d));
}

TEST_CASE("GraphSchedule — deep chain of 5 nodes gives 5 levels",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId n0 = model.addNode("DummySource");
    NodeId n1 = model.addNode("DummyProcess");
    NodeId n2 = model.addNode("DummyProcess");
    NodeId n3 = model.addNode("DummyProcess");
    NodeId n4 = model.addNode("DummySink");

    model.addConnection(ConnectionId{n0, 0, n1, 0});
    model.addConnection(ConnectionId{n1, 0, n2, 0});
    model.addConnection(ConnectionId{n2, 0, n3, 0});
    model.addConnection(ConnectionId{n3, 0, n4, 0});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());
    REQUIRE(schedule.levels.size() == 5);

    for (std::size_t i = 0; i < 5; ++i) {
        NodeId expected = (i == 0) ? n0 : (i == 1) ? n1 : (i == 2) ? n2 : (i == 3) ? n3 : n4;
        REQUIRE(schedule.levels[i].size() == 1);
        REQUIRE(schedule.levels[i][0] == expected);
        REQUIRE(schedule.nodeLevels.at(expected) == i);
    }
}

TEST_CASE("GraphSchedule — source identification excludes connected-input nodes",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId src = model.addNode("DummySource");
    NodeId mid = model.addNode("DummyProcess");
    NodeId snk = model.addNode("DummySink");

    model.addConnection(ConnectionId{src, 0, mid, 0});
    model.addConnection(ConnectionId{mid, 0, snk, 0});

    auto schedule = noddle::buildSchedule(model);

    // Only src is a source node
    REQUIRE(schedule.sourceNodes.size() == 1);
    REQUIRE(schedule.sourceNodes[0] == src);

    // mid and snk are NOT sources
    auto srcSet = toSet(schedule.sourceNodes);
    REQUIRE_FALSE(srcSet.count(mid));
    REQUIRE_FALSE(srcSet.count(snk));
}

TEST_CASE("GraphSchedule — adjacency map contains correct edges",
          "[engine][schedule]")
{
    auto reg = makeTestRegistry();
    DataFlowGraphModel model(reg);

    NodeId a = model.addNode("DummySource");
    NodeId b = model.addNode("DummyProcess");
    NodeId c = model.addNode("DummySink");

    model.addConnection(ConnectionId{a, 0, b, 0});
    model.addConnection(ConnectionId{b, 0, c, 0});

    auto schedule = noddle::buildSchedule(model);

    REQUIRE(schedule.isValid());

    // A has 1 outgoing edge → B
    REQUIRE(schedule.adjacency.count(a));
    REQUIRE(schedule.adjacency.at(a).size() == 1);
    REQUIRE(schedule.adjacency.at(a)[0].targetNode == b);
    REQUIRE(schedule.adjacency.at(a)[0].outPort == 0);
    REQUIRE(schedule.adjacency.at(a)[0].inPort == 0);

    // B has 1 outgoing edge → C
    REQUIRE(schedule.adjacency.count(b));
    REQUIRE(schedule.adjacency.at(b).size() == 1);
    REQUIRE(schedule.adjacency.at(b)[0].targetNode == c);

    // C (sink) has no outgoing edges
    auto cIt = schedule.adjacency.find(c);
    bool cHasNoEdges = (cIt == schedule.adjacency.end()) || cIt->second.empty();
    REQUIRE(cHasNoEdges);
}

// ═══════════════════════════════════════════════════════════════
//  FrameDispatcher tests
// ═══════════════════════════════════════════════════════════════

TEST_CASE("FrameDispatcher — default policy is DropFrame",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd;
    REQUIRE(fd.policy() == noddle::OverloadPolicy::DropFrame);
}

TEST_CASE("FrameDispatcher — DropFrame: first frame accepted",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::DropFrame);
    REQUIRE(fd.acceptFrame() == true);
}

TEST_CASE("FrameDispatcher — DropFrame: second frame while in-flight rejected",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::DropFrame);
    REQUIRE(fd.acceptFrame() == true);  // first frame in-flight
    REQUIRE(fd.acceptFrame() == false); // dropped
}

TEST_CASE("FrameDispatcher — DropFrame: frame accepted after markCompleted",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::DropFrame);
    REQUIRE(fd.acceptFrame() == true);
    fd.markCompleted();
    REQUIRE(fd.acceptFrame() == true);
}

TEST_CASE("FrameDispatcher — Downsample ratio=2 accepts every 2nd frame",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::Downsample);
    fd.setDownsampleRatio(2);

    // Frame 1: accepted (first is always accepted), then complete it
    REQUIRE(fd.acceptFrame() == true);
    fd.markCompleted();

    // Frame 2: skipped by ratio
    REQUIRE(fd.acceptFrame() == false);

    // Frame 3: accepted
    REQUIRE(fd.acceptFrame() == true);
    fd.markCompleted();

    // Frame 4: skipped
    REQUIRE(fd.acceptFrame() == false);

    // Frame 5: accepted
    REQUIRE(fd.acceptFrame() == true);
}

TEST_CASE("FrameDispatcher — Downsample: drops even if ratio allows when in-flight",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::Downsample);
    fd.setDownsampleRatio(1); // accept every frame by ratio

    REQUIRE(fd.acceptFrame() == true); // in-flight now
    // Ratio allows it but previous not completed → drop
    REQUIRE(fd.acceptFrame() == false);
}

TEST_CASE("FrameDispatcher — Pause: first frame accepted",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::Pause);
    REQUIRE(fd.acceptFrame() == true);
}

TEST_CASE("FrameDispatcher — Pause: shouldPauseSource while in-flight",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::Pause);
    REQUIRE(fd.acceptFrame() == true);
    REQUIRE(fd.shouldPauseSource() == true);
}

TEST_CASE("FrameDispatcher — Pause: shouldPauseSource false after markCompleted",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::Pause);
    REQUIRE(fd.acceptFrame() == true);
    REQUIRE(fd.shouldPauseSource() == true);
    fd.markCompleted();
    REQUIRE(fd.shouldPauseSource() == false);
}

TEST_CASE("FrameDispatcher — policy change at runtime updates behavior",
          "[engine][backpressure]")
{
    noddle::FrameDispatcher fd(noddle::OverloadPolicy::DropFrame);
    REQUIRE(fd.policy() == noddle::OverloadPolicy::DropFrame);

    fd.setPolicy(noddle::OverloadPolicy::Pause);
    REQUIRE(fd.policy() == noddle::OverloadPolicy::Pause);

    // Verify Pause behavior is active after policy change
    REQUIRE(fd.acceptFrame() == true);
    REQUIRE(fd.shouldPauseSource() == true);
}
