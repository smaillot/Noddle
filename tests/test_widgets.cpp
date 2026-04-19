#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../app/widgets/PipelineProfiler.hpp"
#include "../app/widgets/FpsCounter.hpp"
#include "../app/widgets/ProcessingTimer.hpp"

#include <thread>
#include <chrono>

using Catch::Approx;

// Shorthand aliases
using SP = PipelineProfiler::StatType;
using TW = PipelineProfiler::TimeWindow;

// ── PipelineProfiler ────────────────────────────────────────────

TEST_CASE("PipelineProfiler — singleton returns same instance", "[profiler]") {
    auto &a = PipelineProfiler::instance();
    auto &b = PipelineProfiler::instance();
    REQUIRE(&a == &b);
}

TEST_CASE("PipelineProfiler — stat returns 0 for unknown stage", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();
    REQUIRE(p.stat("unknown", "stage", SP::Avg, TW::AllTime) == 0.0);
}

TEST_CASE("PipelineProfiler — record and stat Avg AllTime", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 10.0);
    p.record("NodeA", "compute", 20.0);
    p.record("NodeA", "compute", 30.0);

    double avg = p.stat("NodeA", "compute", SP::Avg, TW::AllTime);
    REQUIRE(avg == Approx(20.0));
}

TEST_CASE("PipelineProfiler — stat Min returns minimum value", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 15.0);
    p.record("NodeA", "compute", 5.0);
    p.record("NodeA", "compute", 25.0);

    REQUIRE(p.stat("NodeA", "compute", SP::Min, TW::AllTime) == Approx(5.0));
}

TEST_CASE("PipelineProfiler — stat Max returns maximum value", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 15.0);
    p.record("NodeA", "compute", 5.0);
    p.record("NodeA", "compute", 25.0);

    REQUIRE(p.stat("NodeA", "compute", SP::Max, TW::AllTime) == Approx(25.0));
}

TEST_CASE("PipelineProfiler — stat Median with odd count", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 30.0);
    p.record("NodeA", "compute", 10.0);
    p.record("NodeA", "compute", 20.0);

    // Sorted: 10, 20, 30 → median = 20
    REQUIRE(p.stat("NodeA", "compute", SP::Median, TW::AllTime) == Approx(20.0));
}

TEST_CASE("PipelineProfiler — stat Median with even count", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 10.0);
    p.record("NodeA", "compute", 40.0);
    p.record("NodeA", "compute", 20.0);
    p.record("NodeA", "compute", 30.0);

    // Sorted: 10, 20, 30, 40 → median = (20+30)/2 = 25
    REQUIRE(p.stat("NodeA", "compute", SP::Median, TW::AllTime) == Approx(25.0));
}

TEST_CASE("PipelineProfiler — snapshot returns stages in recording order", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeB", "decode", 5.0);
    p.record("NodeA", "compute", 10.0);
    p.record("NodeC", "encode", 15.0);

    auto snap = p.snapshot(SP::Avg, TW::AllTime);
    REQUIRE(snap.size() == 3);
    REQUIRE(snap[0].nodeCaption == "NodeB");
    REQUIRE(snap[0].stageName == "decode");
    REQUIRE(snap[0].durationMs == Approx(5.0));
    REQUIRE(snap[1].nodeCaption == "NodeA");
    REQUIRE(snap[1].stageName == "compute");
    REQUIRE(snap[2].nodeCaption == "NodeC");
    REQUIRE(snap[2].stageName == "encode");
}

TEST_CASE("PipelineProfiler — totalPipelineMs sums all stages", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 10.0);
    p.record("NodeB", "decode", 20.0);
    p.record("NodeC", "encode", 30.0);

    double total = p.totalPipelineMs(SP::Avg, TW::AllTime);
    REQUIRE(total == Approx(60.0));
}

TEST_CASE("PipelineProfiler — TimeWindow LastFrame filters by current frame", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    // Frame 0 (initial)
    p.record("NodeA", "compute", 100.0);
    p.markFrame("NodeA");
    // Frame 1
    p.record("NodeA", "compute", 42.0);

    // LastFrame should only see frame 1
    double val = p.stat("NodeA", "compute", SP::Avg, TW::LastFrame);
    REQUIRE(val == Approx(42.0));
}

TEST_CASE("PipelineProfiler — TimeWindow Frames10 filters recent frames", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    // Record across 15 frames
    for (int i = 0; i < 15; ++i) {
        p.markFrame("NodeA");
        p.record("NodeA", "compute", double(i));
    }

    // Frames10: lastFrame=15, cutoff=5, frameIds 6..15 → values 5..14
    // Avg = (5+6+7+8+9+10+11+12+13+14) / 10 = 9.5
    double avg = p.stat("NodeA", "compute", SP::Avg, TW::Frames10);
    REQUIRE(avg == Approx(9.5));
}

TEST_CASE("PipelineProfiler — timelineEvents returns chronologically sorted events", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "stage1", 5.0);
    p.record("NodeB", "stage2", 10.0);
    p.record("NodeC", "stage3", 15.0);

    // Large window to capture all entries regardless of timer offset
    auto events = p.timelineEvents(100000);
    REQUIRE(events.size() == 3);

    // Verify chronological ordering
    for (int i = 1; i < events.size(); ++i) {
        REQUIRE(events[i].timestampMs >= events[i - 1].timestampMs);
    }
}

TEST_CASE("PipelineProfiler — clear resets all data", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    p.record("NodeA", "compute", 10.0);
    REQUIRE(p.snapshot(SP::Avg, TW::AllTime).size() == 1);

    p.clear();

    REQUIRE(p.snapshot(SP::Avg, TW::AllTime).isEmpty());
    REQUIRE(p.stat("NodeA", "compute", SP::Avg, TW::AllTime) == 0.0);
    REQUIRE(p.totalPipelineMs(SP::Avg, TW::AllTime) == 0.0);
}

TEST_CASE("PipelineProfiler — ScopeStageTimer records on destruction", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    {
        ScopeStageTimer timer("TestNode", "scopedStage");
        // Timer runs during this scope
    }

    // Timer destroyed — should have recorded an entry
    auto snap = p.snapshot(SP::Avg, TW::AllTime);
    REQUIRE(snap.size() == 1);
    REQUIRE(snap[0].nodeCaption == "TestNode");
    REQUIRE(snap[0].stageName == "scopedStage");
    REQUIRE(snap[0].durationMs >= 0.0);
}

TEST_CASE("PipelineProfiler — thread safety with concurrent writers", "[profiler]") {
    auto &p = PipelineProfiler::instance();
    p.clear();

    constexpr int kIterations = 200;

    auto worker = [&](QString prefix) {
        for (int i = 0; i < kIterations; ++i)
            p.record(prefix, "stage", double(i));
    };

    std::thread t1(worker, QString("Thread1"));
    std::thread t2(worker, QString("Thread2"));
    t1.join();
    t2.join();

    // No crash = mutex works. Verify both threads' data is present.
    auto snap = p.snapshot(SP::Avg, TW::AllTime);
    REQUIRE(snap.size() == 2);

    // Each thread recorded kIterations entries; avg = (0+1+...+199)/200 = 99.5
    double avg1 = p.stat("Thread1", "stage", SP::Avg, TW::AllTime);
    double avg2 = p.stat("Thread2", "stage", SP::Avg, TW::AllTime);
    REQUIRE(avg1 == Approx(99.5));
    REQUIRE(avg2 == Approx(99.5));
}

// ── FpsCounter ──────────────────────────────────────────────────
// NOTE: FpsCounter relies on QElapsedTimer for real-time measurement.
// Accurate FPS testing would require 1+ second of real time.
// We test default state and basic non-crash behavior.

TEST_CASE("FpsCounter — initial fps is 0", "[fps]") {
    FpsCounter counter;
    REQUIRE(counter.fps() == 0.0);
}

TEST_CASE("FpsCounter — fps remains 0 before 1s threshold", "[fps]") {
    // FpsCounter only updates m_fps after 1 second has elapsed.
    // Without sleeping, the threshold won't be reached.
    FpsCounter counter;
    for (int i = 0; i < 10; ++i)
        counter.tick();

    REQUIRE(counter.fps() == 0.0);
}

// ── ProcessingTimer ─────────────────────────────────────────────
// NOTE: ProcessingTimer relies on QElapsedTimer for real-time measurement.
// avgMs is only updated after 10 start/stop cycles (batched averaging).

TEST_CASE("ProcessingTimer — initial avgMs is 0", "[timer]") {
    ProcessingTimer timer;
    REQUIRE(timer.avgMs() == 0.0);
}

TEST_CASE("ProcessingTimer — avgMs positive after 10 start/stop cycles", "[timer]") {
    ProcessingTimer timer;
    for (int i = 0; i < 10; ++i) {
        timer.start();
        // Small busy-wait to ensure measurable elapsed time
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        timer.stop();
    }

    // After 10 cycles, avgMs should be updated and positive
    REQUIRE(timer.avgMs() > 0.0);
}
