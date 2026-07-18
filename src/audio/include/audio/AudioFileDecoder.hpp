#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace audio {

// Decodes an audio file to interleaved 32-bit float PCM at a target sample rate
// and channel count, using FFmpeg (libavformat/libavcodec/libswresample).
//
// Role: this is the PRODUCER that feeds AudioDeck's ring buffer. It runs on the
// decode thread — NOT on the real-time audio thread. It is allowed to allocate
// and block here (the RT contract in ADR-0004 applies only to the callback).
//
// Usage:
//   AudioFileDecoder dec;
//   if (dec.open("track.wav", 48000, 2)) {
//       std::vector<float> buf(4096 * 2);
//       for (;;) {
//           std::size_t frames = dec.readFrames(buf.data(), 4096);
//           if (frames == 0) break;          // EOF
//           deck.feed(buf.data(), frames);   // push into the ring buffer
//       }
//   }
//
// The FFmpeg dependency is confined to the .cpp so this header stays clean and
// includable from the pure core and tests.
class AudioFileDecoder {
public:
    AudioFileDecoder();
    ~AudioFileDecoder();

    AudioFileDecoder(const AudioFileDecoder&) = delete;
    AudioFileDecoder& operator=(const AudioFileDecoder&) = delete;

    // Opens `path` and prepares conversion to `targetSampleRate` / `targetChannels`
    // interleaved float. Returns false on failure (missing file, no audio stream,
    // unsupported codec); lastError() explains why.
    bool open(const std::string& path, int targetSampleRate, int targetChannels);

    // Fills `out` with up to `maxFrames` interleaved frames (targetChannels()
    // samples per frame). Returns the number of frames written; 0 means EOF.
    // `out` must hold at least maxFrames * targetChannels() floats.
    std::size_t readFrames(float* out, std::size_t maxFrames);

    // Rewinds to the start (for looping without reopening).
    bool seekToStart();

    bool isOpen() const { return impl_ != nullptr; }
    int targetSampleRate() const { return targetSampleRate_; }
    int targetChannels() const { return targetChannels_; }

    // Source stream properties (as reported by the container), valid after open().
    int sourceSampleRate() const { return sourceSampleRate_; }
    int sourceChannels() const { return sourceChannels_; }

    const std::string& lastError() const { return lastError_; }

private:
    struct Impl;          // hides all FFmpeg types
    Impl* impl_ = nullptr;

    int targetSampleRate_ = 0;
    int targetChannels_ = 0;
    int sourceSampleRate_ = 0;
    int sourceChannels_ = 0;
    std::string lastError_;
};

} // namespace audio
