#include "audio/AudioDeck.hpp"
#include "audio/DeckStateMachine.hpp"
#include "audio/GainRamp.hpp"
#include "audio/RingBuffer.hpp"

#ifdef ATIVASTAGE_HAVE_FFMPEG
#include "audio/AudioFileDecoder.hpp"
#include <string>
#endif

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace audio;

// --------------------------------------------------------------------------
// DeckStateMachine
// --------------------------------------------------------------------------
TEST_CASE("Deck follows the legal play/pause/stop path", "[audio][state]") {
    DeckStateMachine sm;
    REQUIRE(sm.state() == DeckState::Ready);

    REQUIRE(sm.load());
    REQUIRE(sm.state() == DeckState::Loading);
    REQUIRE(sm.onLoaded());
    REQUIRE(sm.state() == DeckState::Ready);

    REQUIRE(sm.play());
    REQUIRE(sm.state() == DeckState::Playing);
    REQUIRE(sm.pause());
    REQUIRE(sm.state() == DeckState::Paused);
    REQUIRE(sm.play());
    REQUIRE(sm.stop());
    REQUIRE(sm.state() == DeckState::Ready);
}

TEST_CASE("Deck rejects illegal transitions", "[audio][state]") {
    DeckStateMachine sm;
    // Cannot pause or fade a deck that is not playing.
    REQUIRE_FALSE(sm.pause());
    REQUIRE_FALSE(sm.fadeOut());
    // Cannot go straight from Ready to Paused.
    REQUIRE(sm.state() == DeckState::Ready);
}

TEST_CASE("Fade-out path returns to Ready", "[audio][state]") {
    DeckStateMachine sm;
    REQUIRE(sm.play());
    REQUIRE(sm.fadeOut());
    REQUIRE(sm.state() == DeckState::FadingOut);
    REQUIRE(sm.onFadeComplete());
    REQUIRE(sm.state() == DeckState::Ready);
}

TEST_CASE("Failure is reachable from any state and recoverable", "[audio][state]") {
    DeckStateMachine sm;
    REQUIRE(sm.play());
    REQUIRE(sm.fail());
    REQUIRE(sm.state() == DeckState::Failed);
    REQUIRE(sm.reset());
    REQUIRE(sm.state() == DeckState::Ready);
}

TEST_CASE("Deck state labels are the report's pt-BR strings", "[audio][state]") {
    REQUIRE(std::string(deckStateLabelPtBr(DeckState::Playing)) == "TOCANDO");
    REQUIRE(std::string(deckStateLabelPtBr(DeckState::FadingOut)) == "FADE OUT");
}

// --------------------------------------------------------------------------
// RingBuffer (SPSC lock-free)
// --------------------------------------------------------------------------
TEST_CASE("RingBuffer rounds capacity to a power of two", "[audio][ring]") {
    RingBuffer<float> rb(1000);
    REQUIRE(rb.capacity() == 1024);
    REQUIRE(rb.availableRead() == 0);
    REQUIRE(rb.availableWrite() == 1024);
}

TEST_CASE("RingBuffer preserves FIFO order and counts", "[audio][ring]") {
    RingBuffer<float> rb(8);
    const std::vector<float> in{1, 2, 3, 4, 5};
    REQUIRE(rb.push(in.data(), in.size()) == 5);
    REQUIRE(rb.availableRead() == 5);

    std::vector<float> out(5, 0.0f);
    REQUIRE(rb.pop(out.data(), 5) == 5);
    REQUIRE(out == in);
    REQUIRE(rb.availableRead() == 0);
}

TEST_CASE("RingBuffer refuses to overflow and wraps correctly", "[audio][ring]") {
    RingBuffer<float> rb(4); // capacity 4
    const std::vector<float> in{1, 2, 3, 4, 5, 6};
    REQUIRE(rb.push(in.data(), in.size()) == 4); // only 4 fit

    std::vector<float> out(2, 0.0f);
    REQUIRE(rb.pop(out.data(), 2) == 2);        // drain 2 -> {1,2}
    REQUIRE(out[0] == 1.0f);
    REQUIRE(out[1] == 2.0f);

    const std::vector<float> more{7, 8};        // now 2 slots free -> wrap
    REQUIRE(rb.push(more.data(), more.size()) == 2);
    REQUIRE(rb.availableRead() == 4);           // {3,4,7,8}
}

// --------------------------------------------------------------------------
// GainRamp
// --------------------------------------------------------------------------
TEST_CASE("GainRamp reaches its target over N frames", "[audio][ramp]") {
    GainRamp ramp(1.0f);
    ramp.setTarget(0.0f, 4);
    REQUIRE_FALSE(ramp.atTarget());
    for (int i = 0; i < 4; ++i) {
        ramp.next();
    }
    REQUIRE(ramp.atTarget());
    REQUIRE(ramp.current() == 0.0f);
}

TEST_CASE("GainRamp with zero frames applies immediately", "[audio][ramp]") {
    GainRamp ramp(1.0f);
    ramp.setTarget(0.5f, 0);
    REQUIRE(ramp.atTarget());
    REQUIRE(ramp.current() == 0.5f);
}

// --------------------------------------------------------------------------
// AudioDeck render path (the real-time callback body)
// --------------------------------------------------------------------------
TEST_CASE("Deck renders silence when not playing", "[audio][deck]") {
    AudioDeck deck(48000, 2, 4096);
    std::vector<float> out(128, 7.0f); // poisoned
    deck.renderInto(out.data(), 64);   // 64 frames * 2 ch
    for (float s : out) {
        REQUIRE(s == 0.0f);
    }
}

TEST_CASE("Deck plays fed samples at unity gain", "[audio][deck]") {
    AudioDeck deck(48000, 2, 4096);
    // Feed 4 stereo frames of value 0.5.
    std::vector<float> pcm(8, 0.5f);
    REQUIRE(deck.feed(pcm.data(), 4) == 4);
    REQUIRE(deck.play());

    std::vector<float> out(8, 0.0f);
    deck.renderInto(out.data(), 4);
    for (float s : out) {
        REQUIRE(s == 0.5f); // unity gain, no fade requested
    }
}

TEST_CASE("Deck underrun is zero-filled, not garbage", "[audio][deck]") {
    AudioDeck deck(48000, 2, 4096);
    std::vector<float> pcm(4, 0.5f); // only 2 stereo frames
    REQUIRE(deck.feed(pcm.data(), 2) == 2);
    REQUIRE(deck.play());

    std::vector<float> out(8, 9.0f); // ask for 4 frames, poisoned
    deck.renderInto(out.data(), 4);
    // First 2 frames present, last 2 zero-filled.
    REQUIRE(out[0] == 0.5f);
    REQUIRE(out[3] == 0.5f);
    REQUIRE(out[4] == 0.0f);
    REQUIRE(out[7] == 0.0f);
}

TEST_CASE("Fade-out drives output to zero and completes", "[audio][deck]") {
    const int sr = 1000; // 1 kHz keeps the math small
    AudioDeck deck(sr, 1, 4096);
    std::vector<float> pcm(64, 1.0f);
    deck.feed(pcm.data(), 64);
    REQUIRE(deck.play());

    REQUIRE(deck.fadeOut(10)); // 10 ms -> 10 frames at 1 kHz
    REQUIRE(deck.state() == DeckState::FadingOut);

    std::vector<float> out(64, 0.0f);
    deck.renderInto(out.data(), 32); // enough frames to finish the fade
    REQUIRE(out.front() > 0.0f);     // starts audible
    REQUIRE(out.back() == 0.0f);     // ends silent

    REQUIRE(deck.pollFadeComplete());
    REQUIRE(deck.state() == DeckState::Ready);
}

// --------------------------------------------------------------------------
// AudioFileDecoder (FFmpeg) — runs only where FFmpeg is available.
// Decodes the committed 48 kHz / stereo / 0.25 s tone fixture.
// --------------------------------------------------------------------------
#ifdef ATIVASTAGE_HAVE_FFMPEG
TEST_CASE("Decoder resamples the tone fixture to interleaved float", "[audio][decode]") {
    const std::string path =
        std::string(AUDIO_TEST_FIXTURE_DIR) + "/tone_48k_stereo.wav";

    AudioFileDecoder dec;
    REQUIRE(dec.open(path, 48000, 2));
    REQUIRE(dec.sourceSampleRate() == 48000);
    REQUIRE(dec.sourceChannels() == 2);
    REQUIRE(dec.targetChannels() == 2);

    std::vector<float> buf(1024 * 2);
    std::size_t total = 0;
    bool nonZeroSeen = false;
    for (;;) {
        const std::size_t frames = dec.readFrames(buf.data(), 1024);
        if (frames == 0) {
            break;
        }
        total += frames;
        for (std::size_t i = 0; i < frames * 2; ++i) {
            if (buf[i] != 0.0f) {
                nonZeroSeen = true;
            }
        }
    }
    // ~0.25 s at 48 kHz ≈ 12000 frames; allow slack for resampler edges.
    REQUIRE(total > 10000);
    REQUIRE(total < 14000);
    REQUIRE(nonZeroSeen);
}

TEST_CASE("Decoder resamples to a different rate and mono", "[audio][decode]") {
    const std::string path =
        std::string(AUDIO_TEST_FIXTURE_DIR) + "/tone_48k_stereo.wav";

    AudioFileDecoder dec;
    REQUIRE(dec.open(path, 44100, 1)); // 48k stereo -> 44.1k mono
    REQUIRE(dec.targetChannels() == 1);

    std::vector<float> buf(2048);
    std::size_t total = 0;
    for (;;) {
        const std::size_t frames = dec.readFrames(buf.data(), 2048);
        if (frames == 0) {
            break;
        }
        total += frames;
    }
    // 0.25 s at 44.1 kHz ≈ 11025 frames.
    REQUIRE(total > 9000);
    REQUIRE(total < 13000);
}

TEST_CASE("Decoder seekToStart allows re-reading (loop)", "[audio][decode]") {
    const std::string path =
        std::string(AUDIO_TEST_FIXTURE_DIR) + "/tone_48k_stereo.wav";

    AudioFileDecoder dec;
    REQUIRE(dec.open(path, 48000, 2));

    std::vector<float> buf(1024 * 2);
    std::size_t first = 0;
    while (std::size_t f = dec.readFrames(buf.data(), 1024)) {
        first += f;
    }
    REQUIRE(first > 10000);

    REQUIRE(dec.seekToStart());
    std::size_t second = 0;
    while (std::size_t f = dec.readFrames(buf.data(), 1024)) {
        second += f;
    }
    REQUIRE(second > 10000);
}
#endif // ATIVASTAGE_HAVE_FFMPEG
