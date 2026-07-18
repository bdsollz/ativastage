// audio_probe — manual sanity tool for Fase 1, Marco 1.1.
//
// Plays an audio file end to end through the real pipeline:
//   AudioFileDecoder (FFmpeg)  ->  AudioDeck ring buffer  ->  AudioOutput (miniaudio)
//
// This is NOT an automated test (it needs a real output device and speakers);
// it exists so the Marco 1.1 audio path can be verified by ear on macOS and
// Windows. Build target is only created when both FFmpeg and miniaudio are
// available.
//
// Usage:
//   audio_probe <file> [--fade-ms N]
//     plays <file>; if --fade-ms is given, fades out N ms before the end.

#include "audio/AudioDeck.hpp"
#include "audio/AudioFileDecoder.hpp"
#include "audio/AudioOutput.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <audio-file> [--fade-ms N]\n", argv[0]);
        return 2;
    }
    const std::string path = argv[1];
    int fadeMs = 0;
    for (int i = 2; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "--fade-ms") == 0) {
            fadeMs = std::atoi(argv[i + 1]);
        }
    }

    const int sampleRate = 48000;
    const int channels = 2;

    audio::AudioFileDecoder decoder;
    if (!decoder.open(path, sampleRate, channels)) {
        std::fprintf(stderr, "decode open failed: %s\n", decoder.lastError().c_str());
        return 1;
    }
    std::printf("source: %d Hz, %d ch -> output: %d Hz, %d ch\n",
                decoder.sourceSampleRate(), decoder.sourceChannels(),
                sampleRate, channels);

    // ~1 s of ring headroom.
    audio::AudioDeck deck(sampleRate, channels, sampleRate);

    audio::AudioOutput out;
    if (!out.start(&deck, sampleRate, channels)) {
        std::fprintf(stderr, "audio output failed: %s\n", out.lastError().c_str());
        return 1;
    }
    deck.play();

    // Producer thread: decode and feed the ring buffer, blocking when it is full.
    std::atomic<bool> eof{false};
    std::thread producer([&] {
        std::vector<float> buf(1024 * channels);
        for (;;) {
            const std::size_t frames = decoder.readFrames(buf.data(), 1024);
            if (frames == 0) {
                eof.store(true);
                break;
            }
            std::size_t offset = 0;
            while (offset < frames) {
                const std::size_t fed =
                    deck.feed(buf.data() + offset * channels, frames - offset);
                offset += fed;
                if (fed == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
        }
    });

    // Control loop: wait for EOF, optionally fade, then let the ring drain.
    while (!eof.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        deck.pollFadeComplete();
    }
    if (fadeMs > 0) {
        deck.fadeOut(fadeMs);
        for (int i = 0; i < fadeMs / 20 + 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            if (deck.pollFadeComplete()) {
                break;
            }
        }
    } else {
        // Give the ring time to empty at the end.
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    }

    producer.join();
    out.stop();
    std::printf("done.\n");
    return 0;
}
