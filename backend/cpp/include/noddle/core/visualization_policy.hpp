#pragma once

#include <optional>

namespace noddle::core {

enum class FrameKind {
    Stream,
    SingleFrame,
};

enum class OverloadStrategy {
    DropFrame,
    Downsample,
    Pause,
};

struct VisualizationPolicy {
    bool realtime_enabled {true};
    OverloadStrategy stream_default {OverloadStrategy::DropFrame};
    OverloadStrategy single_frame_default {OverloadStrategy::Pause};

    OverloadStrategy resolve(
        FrameKind frame_kind,
        const std::optional<OverloadStrategy>& user_override = std::nullopt
    ) const {
        if (user_override.has_value()) {
            return *user_override;
        }
        return frame_kind == FrameKind::Stream ? stream_default : single_frame_default;
    }
};

} // namespace noddle::core
