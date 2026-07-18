#pragma once

#include <cstddef>

namespace audio {

// Per-frame linear gain ramp used to apply volume changes and fades inside the
// real-time callback WITHOUT clicks. Real-time safe: no allocation, no locks —
// just a few floats advanced one frame at a time.
//
// A fade-out is simply setTarget(0.0f, fadeFrames); the deck detects completion
// via atTarget() with a target of 0. Log/equal-power curves are a later
// refinement (ADR-0004 lists linear as the MVP curve); the interface stays the
// same.
class GainRamp {
public:
    GainRamp() = default;
    explicit GainRamp(float initialGain) : current_(initialGain), target_(initialGain) {}

    // Ramp toward `target` over `frames`. frames == 0 applies immediately.
    void setTarget(float target, std::size_t frames) {
        target_ = target;
        if (frames == 0) {
            current_ = target;
            framesRemaining_ = 0;
            step_ = 0.0f;
            return;
        }
        framesRemaining_ = frames;
        step_ = (target_ - current_) / static_cast<float>(frames);
    }

    // Advances one frame and returns the gain to apply to that frame's samples.
    float next() {
        const float g = current_;
        if (framesRemaining_ > 0) {
            current_ += step_;
            if (--framesRemaining_ == 0) {
                current_ = target_; // snap to avoid float drift
            }
        }
        return g;
    }

    float current() const { return current_; }
    float target() const { return target_; }
    bool atTarget() const { return framesRemaining_ == 0; }

private:
    float       current_ = 1.0f;
    float       target_ = 1.0f;
    float       step_ = 0.0f;
    std::size_t framesRemaining_ = 0;
};

} // namespace audio
