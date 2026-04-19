#pragma once

#include <atomic>
#include <cstdint>

namespace noddle {

enum class OverloadPolicy { DropFrame, Downsample, Pause };

class FrameDispatcher
{
public:
    explicit FrameDispatcher(OverloadPolicy policy = OverloadPolicy::DropFrame)
        : m_policy(policy) {}

    void setPolicy(OverloadPolicy policy) { m_policy.store(policy, std::memory_order_relaxed); }
    [[nodiscard]] OverloadPolicy policy() const { return m_policy.load(std::memory_order_relaxed); }

    [[nodiscard]] bool acceptFrame() {
        auto p = m_policy.load(std::memory_order_relaxed);
        switch (p) {
        case OverloadPolicy::DropFrame:
        case OverloadPolicy::Pause:
            return !m_inFlight.exchange(true, std::memory_order_acq_rel);

        case OverloadPolicy::Downsample: {
            auto count = m_frameCounter.fetch_add(1, std::memory_order_relaxed);
            auto ratio = m_downsampleRatio.load(std::memory_order_relaxed);
            if (ratio == 0) ratio = 1;
            if (count % ratio != 0) return false;
            return !m_inFlight.exchange(true, std::memory_order_acq_rel);
        }
        }
        return false;
    }

    void markCompleted() { m_inFlight.store(false, std::memory_order_release); }

    void setDownsampleRatio(unsigned int ratio) {
        m_downsampleRatio.store(ratio, std::memory_order_relaxed);
    }

    [[nodiscard]] bool shouldPauseSource() const {
        return m_inFlight.load(std::memory_order_acquire);
    }

private:
    std::atomic<OverloadPolicy> m_policy;
    std::atomic<bool> m_inFlight{false};
    std::atomic<uint64_t> m_frameCounter{0};
    std::atomic<unsigned int> m_downsampleRatio{2};
};

} // namespace noddle
