#pragma once

#include "audio/DeckState.hpp"
#include "audio/DeckStateMachine.hpp"
#include "audio/GainRamp.hpp"
#include "audio/RingBuffer.hpp"

#include <atomic>
#include <cstddef>

namespace audio {

// One audio deck: the minimal player required by Fase 1, Marco 1.1
// (play / pause / stop / volume / fade-out). It ties together the state
// machine, the SPSC ring buffer, and the gain ramp.
//
// Three roles touch a deck, on three threads:
//   * control thread — play()/pause()/stop()/setVolume()/fadeOut(): mutate the
//     DeckStateMachine and publish intent via atomics.
//   * decode thread (producer) — feed(): pushes decoded interleaved PCM into
//     the ring buffer. (Wired to the FFmpeg decoder when real playback lands.)
//   * audio thread (consumer) — renderInto(): the real-time callback body.
//     RT-safe: no allocation, no locks, no I/O, no SQLite (ADR-0004). It only
//     reads atomics, pops the ring buffer, and applies the gain ramp.
//
// The audio thread never inspects DeckState directly; it reads the atomic
// `playing_` gate and the atomic fade request, so control-thread state changes
// cannot race the render.
class AudioDeck {
public:
    AudioDeck(int sampleRate, int channels, std::size_t ringCapacityFrames);

    // ---- control thread -------------------------------------------------
    DeckState state() const { return sm_.state(); }

    bool play();
    bool pause();
    bool stop();

    // Target output gain (linear, 1.0 == unity). Applied via a short ramp so it
    // never clicks.
    void setVolume(float linearGain);

    // Playing -> FadingOut over `fadeMs`. When the ramp reaches zero the deck
    // auto-transitions FadingOut -> Ready on the next poll of the control side.
    bool fadeOut(int fadeMs);

    // Must be called from the control thread periodically (or after render) to
    // finalize a completed fade. Returns true when a fade just completed.
    bool pollFadeComplete();

    // ---- decode thread (producer) --------------------------------------
    // Pushes interleaved PCM frames; returns frames actually accepted.
    std::size_t feed(const float* interleaved, std::size_t frames);

    // ---- audio thread (consumer, real-time safe) -----------------------
    // Fills `out` with `frames` interleaved frames (channels() per frame).
    void renderInto(float* out, std::size_t frames);

    int sampleRate() const { return sampleRate_; }
    int channels() const { return channels_; }

private:
    int sampleRate_;
    int channels_;

    DeckStateMachine   sm_;
    RingBuffer<float>  ring_;   // interleaved PCM
    GainRamp           ramp_;   // owned by the audio thread

    // Control <-> audio communication.
    std::atomic<bool>  playing_{false};
    std::atomic<float> targetGain_{1.0f};
    std::atomic<int>   fadeFramesReq_{-1}; // >=0 requests a fade over N frames
    std::atomic<bool>  fadeDone_{false};
};

} // namespace audio
