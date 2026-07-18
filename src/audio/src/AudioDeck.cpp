#include "audio/AudioDeck.hpp"

#include <algorithm>

namespace audio {

AudioDeck::AudioDeck(int sampleRate, int channels, std::size_t ringCapacityFrames)
    : sampleRate_(sampleRate),
      channels_(channels),
      ring_(ringCapacityFrames * static_cast<std::size_t>(channels)),
      ramp_(1.0f) {}

bool AudioDeck::play() {
    if (sm_.play()) {
        playing_.store(true, std::memory_order_release);
        return true;
    }
    return false;
}

bool AudioDeck::pause() {
    if (sm_.pause()) {
        playing_.store(false, std::memory_order_release);
        return true;
    }
    return false;
}

bool AudioDeck::stop() {
    if (sm_.stop()) {
        playing_.store(false, std::memory_order_release);
        ring_.clear();
        return true;
    }
    return false;
}

void AudioDeck::setVolume(float linearGain) {
    targetGain_.store(std::max(0.0f, linearGain), std::memory_order_release);
    // Non-fade volume changes ramp over a short, fixed window (~10 ms) to avoid
    // clicks; requested here and picked up by the render.
    fadeFramesReq_.store(sampleRate_ / 100, std::memory_order_release);
}

bool AudioDeck::fadeOut(int fadeMs) {
    if (sm_.fadeOut()) {
        const int frames = std::max(1, sampleRate_ * fadeMs / 1000);
        targetGain_.store(0.0f, std::memory_order_release);
        fadeFramesReq_.store(frames, std::memory_order_release);
        return true;
    }
    return false;
}

bool AudioDeck::pollFadeComplete() {
    if (fadeDone_.exchange(false, std::memory_order_acq_rel)) {
        playing_.store(false, std::memory_order_release);
        return sm_.onFadeComplete();
    }
    return false;
}

std::size_t AudioDeck::feed(const float* interleaved, std::size_t frames) {
    const std::size_t samples = frames * static_cast<std::size_t>(channels_);
    const std::size_t written = ring_.push(interleaved, samples);
    return written / static_cast<std::size_t>(channels_);
}

void AudioDeck::renderInto(float* out, std::size_t frames) {
    const std::size_t ch = static_cast<std::size_t>(channels_);
    const std::size_t wanted = frames * ch;

    // Not playing: emit silence. RT-safe.
    if (!playing_.load(std::memory_order_acquire)) {
        std::fill(out, out + wanted, 0.0f);
        return;
    }

    // Pick up a pending gain/fade request (published by the control thread).
    const int req = fadeFramesReq_.exchange(-1, std::memory_order_acq_rel);
    if (req >= 0) {
        ramp_.setTarget(targetGain_.load(std::memory_order_acquire),
                        static_cast<std::size_t>(req));
    }

    // Pull decoded PCM; zero-fill on underrun so we never emit garbage.
    const std::size_t got = ring_.pop(out, wanted);
    if (got < wanted) {
        std::fill(out + got, out + wanted, 0.0f);
    }

    // Apply the per-frame gain ramp (same gain across channels within a frame).
    bool reachedZero = false;
    for (std::size_t f = 0; f < frames; ++f) {
        const float g = ramp_.next();
        for (std::size_t c = 0; c < ch; ++c) {
            out[f * ch + c] *= g;
        }
        if (ramp_.atTarget() && ramp_.target() == 0.0f) {
            reachedZero = true;
        }
    }

    // Signal a completed fade-out for the control thread to finalize.
    if (reachedZero) {
        fadeDone_.store(true, std::memory_order_release);
    }
}

} // namespace audio
