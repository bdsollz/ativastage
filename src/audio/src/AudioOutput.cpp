#include "audio/AudioOutput.hpp"

#include "audio/AudioDeck.hpp"

// miniaudio is header-only: define the implementation here, in exactly one TU.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace audio {

struct AudioOutput::Impl {
    ma_device device{};
    bool deviceInited = false;
};

namespace {

// Real-time audio thread. Pulls interleaved float straight from the deck.
// RT-safe: renderInto() does not allocate, lock, or perform I/O (ADR-0004).
void dataCallback(ma_device* device, void* out, const void* /*in*/,
                  ma_uint32 frameCount) {
    auto* deck = static_cast<AudioDeck*>(device->pUserData);
    if (deck != nullptr) {
        deck->renderInto(static_cast<float*>(out),
                         static_cast<std::size_t>(frameCount));
    } else {
        // No deck attached: emit silence.
        const ma_uint32 samples = frameCount * device->playback.channels;
        float* o = static_cast<float*>(out);
        for (ma_uint32 i = 0; i < samples; ++i) {
            o[i] = 0.0f;
        }
    }
}

} // namespace

AudioOutput::AudioOutput() : impl_(new Impl()) {}

AudioOutput::~AudioOutput() {
    stop();
    delete impl_;
    impl_ = nullptr;
}

bool AudioOutput::start(AudioDeck* deck, int sampleRate, int channels) {
    if (running_) {
        return true;
    }
    lastError_.clear();

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = static_cast<ma_uint32>(channels);
    config.sampleRate = static_cast<ma_uint32>(sampleRate);
    config.dataCallback = dataCallback;
    config.pUserData = deck;

    if (ma_device_init(nullptr, &config, &impl_->device) != MA_SUCCESS) {
        lastError_ = "ma_device_init failed (no output device?)";
        return false;
    }
    impl_->deviceInited = true;

    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        lastError_ = "ma_device_start failed";
        ma_device_uninit(&impl_->device);
        impl_->deviceInited = false;
        return false;
    }
    running_ = true;
    return true;
}

void AudioOutput::stop() {
    if (impl_ && impl_->deviceInited) {
        ma_device_uninit(&impl_->device);
        impl_->deviceInited = false;
    }
    running_ = false;
}

} // namespace audio
