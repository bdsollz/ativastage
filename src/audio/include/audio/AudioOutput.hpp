#pragma once

#include <string>

namespace audio {

class AudioDeck;

// Real audio output device (miniaudio: WASAPI on Windows, CoreAudio on macOS).
// Opens a playback device in 32-bit float and runs the real-time callback that
// pulls audio from an AudioDeck via AudioDeck::renderInto().
//
// The data callback is the real-time thread: it obeys ADR-0004 (no allocation,
// no locks, no I/O) because it only calls renderInto(), which itself is RT-safe.
//
// Lifetime: the AudioDeck passed to start() must outlive the AudioOutput (or
// stop() must be called first). Compiled only when miniaudio is available.
class AudioOutput {
public:
    AudioOutput();
    ~AudioOutput();

    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    // Opens the default output device and starts pulling from `deck`.
    // sampleRate/channels must match the deck. Returns false on failure
    // (no device, format unsupported); lastError() explains why.
    bool start(AudioDeck* deck, int sampleRate, int channels);

    // Stops and closes the device. Safe to call if not started.
    void stop();

    bool isRunning() const { return running_; }
    const std::string& lastError() const { return lastError_; }

private:
    struct Impl;      // hides the miniaudio device
    Impl* impl_ = nullptr;
    bool running_ = false;
    std::string lastError_;
};

} // namespace audio
