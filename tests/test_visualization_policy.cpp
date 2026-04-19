#include <catch2/catch_test_macros.hpp>
#include "noddle/core/visualization_policy.hpp"

using namespace noddle::core;

TEST_CASE("VisualizationPolicy — stream default is DropFrame", "[runtime]") {
    VisualizationPolicy policy;
    auto result = policy.resolve(FrameKind::Stream);
    REQUIRE(result == OverloadStrategy::DropFrame);
}

TEST_CASE("VisualizationPolicy — single frame default is Pause", "[runtime]") {
    VisualizationPolicy policy;
    auto result = policy.resolve(FrameKind::SingleFrame);
    REQUIRE(result == OverloadStrategy::Pause);
}

TEST_CASE("VisualizationPolicy — user override takes precedence for stream", "[runtime]") {
    VisualizationPolicy policy;
    auto result = policy.resolve(FrameKind::Stream, OverloadStrategy::Downsample);
    REQUIRE(result == OverloadStrategy::Downsample);
}

TEST_CASE("VisualizationPolicy — user override takes precedence for single frame", "[runtime]") {
    VisualizationPolicy policy;
    auto result = policy.resolve(FrameKind::SingleFrame, OverloadStrategy::DropFrame);
    REQUIRE(result == OverloadStrategy::DropFrame);
}

TEST_CASE("VisualizationPolicy — nullopt override falls back to default", "[runtime]") {
    VisualizationPolicy policy;
    auto result = policy.resolve(FrameKind::Stream, std::nullopt);
    REQUIRE(result == OverloadStrategy::DropFrame);
}

TEST_CASE("VisualizationPolicy — custom defaults are respected", "[runtime]") {
    VisualizationPolicy policy;
    policy.stream_default = OverloadStrategy::Pause;
    policy.single_frame_default = OverloadStrategy::Downsample;

    REQUIRE(policy.resolve(FrameKind::Stream) == OverloadStrategy::Pause);
    REQUIRE(policy.resolve(FrameKind::SingleFrame) == OverloadStrategy::Downsample);
}
