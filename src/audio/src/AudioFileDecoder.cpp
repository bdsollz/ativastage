#include "audio/AudioFileDecoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cstring>

namespace audio {
namespace {

std::string avErr(int code) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(code, buf, sizeof(buf));
    return std::string(buf);
}

} // namespace

// FFmpeg state is fully hidden here so the header carries no libav types.
struct AudioFileDecoder::Impl {
    AVFormatContext* fmt = nullptr;
    AVCodecContext*  dec = nullptr;
    SwrContext*      swr = nullptr;
    AVPacket*        pkt = nullptr;
    AVFrame*         frame = nullptr;
    int              streamIndex = -1;

    std::vector<float> pending;   // converted interleaved float
    std::size_t        cursor = 0; // read offset (in floats) into `pending`
    std::vector<uint8_t> scratch; // swr_convert output scratch
    bool               eofDrained = false;

    ~Impl() {
        if (swr) swr_free(&swr);
        if (frame) av_frame_free(&frame);
        if (pkt) av_packet_free(&pkt);
        if (dec) avcodec_free_context(&dec);
        if (fmt) avformat_close_input(&fmt);
    }
};

AudioFileDecoder::AudioFileDecoder() = default;

AudioFileDecoder::~AudioFileDecoder() {
    delete impl_;
    impl_ = nullptr;
}

bool AudioFileDecoder::open(const std::string& path, int targetSampleRate, int targetChannels) {
    delete impl_;
    impl_ = new Impl();
    targetSampleRate_ = targetSampleRate;
    targetChannels_ = targetChannels;
    lastError_.clear();

    int rc = avformat_open_input(&impl_->fmt, path.c_str(), nullptr, nullptr);
    if (rc < 0) {
        lastError_ = "avformat_open_input: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }
    rc = avformat_find_stream_info(impl_->fmt, nullptr);
    if (rc < 0) {
        lastError_ = "avformat_find_stream_info: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }

    const AVCodec* codec = nullptr;
    impl_->streamIndex =
        av_find_best_stream(impl_->fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (impl_->streamIndex < 0 || codec == nullptr) {
        lastError_ = "no decodable audio stream";
        delete impl_; impl_ = nullptr; return false;
    }

    AVStream* stream = impl_->fmt->streams[impl_->streamIndex];
    impl_->dec = avcodec_alloc_context3(codec);
    if (!impl_->dec) {
        lastError_ = "avcodec_alloc_context3 failed";
        delete impl_; impl_ = nullptr; return false;
    }
    rc = avcodec_parameters_to_context(impl_->dec, stream->codecpar);
    if (rc < 0) {
        lastError_ = "avcodec_parameters_to_context: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }
    rc = avcodec_open2(impl_->dec, codec, nullptr);
    if (rc < 0) {
        lastError_ = "avcodec_open2: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }

    sourceSampleRate_ = impl_->dec->sample_rate;
    sourceChannels_ = impl_->dec->ch_layout.nb_channels;

    // Resampler: source format/layout/rate -> interleaved float at target.
    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, targetChannels_);
    rc = swr_alloc_set_opts2(
        &impl_->swr,
        &outLayout, AV_SAMPLE_FMT_FLT, targetSampleRate_,
        &impl_->dec->ch_layout, impl_->dec->sample_fmt, impl_->dec->sample_rate,
        0, nullptr);
    av_channel_layout_uninit(&outLayout);
    if (rc < 0 || !impl_->swr) {
        lastError_ = "swr_alloc_set_opts2: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }
    rc = swr_init(impl_->swr);
    if (rc < 0) {
        lastError_ = "swr_init: " + avErr(rc);
        delete impl_; impl_ = nullptr; return false;
    }

    impl_->pkt = av_packet_alloc();
    impl_->frame = av_frame_alloc();
    if (!impl_->pkt || !impl_->frame) {
        lastError_ = "av_packet_alloc/av_frame_alloc failed";
        delete impl_; impl_ = nullptr; return false;
    }
    return true;
}

// Converts one decoded frame into interleaved float, appended to `pending`.
static void convertFrame(AudioFileDecoder::Impl* impl, AVFrame* frame,
                         int targetChannels, int targetRate) {
    const int64_t delay = swr_get_delay(impl->swr, frame->sample_rate);
    const int outCount = static_cast<int>(av_rescale_rnd(
        delay + frame->nb_samples, targetRate,
        frame->sample_rate ? frame->sample_rate : targetRate, AV_ROUND_UP));
    const std::size_t needed =
        static_cast<std::size_t>(outCount) * targetChannels * sizeof(float);
    if (impl->scratch.size() < needed) {
        impl->scratch.resize(needed);
    }
    uint8_t* outPtr = impl->scratch.data();
    const int converted = swr_convert(
        impl->swr, &outPtr, outCount,
        const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);
    if (converted > 0) {
        const float* fp = reinterpret_cast<const float*>(impl->scratch.data());
        impl->pending.insert(
            impl->pending.end(), fp,
            fp + static_cast<std::size_t>(converted) * targetChannels);
    }
}

// Pulls and decodes until `pending` gains samples or EOF is fully drained.
// Returns true if any samples were produced.
static bool decodeMore(AudioFileDecoder::Impl* impl, int targetChannels, int targetRate) {
    if (impl->eofDrained) {
        return false;
    }
    for (;;) {
        int rc = av_read_frame(impl->fmt, impl->pkt);
        if (rc < 0) {
            // EOF: flush the decoder, then the resampler.
            avcodec_send_packet(impl->dec, nullptr);
            while (avcodec_receive_frame(impl->dec, impl->frame) == 0) {
                convertFrame(impl, impl->frame, targetChannels, targetRate);
                av_frame_unref(impl->frame);
            }
            uint8_t* outPtr = impl->scratch.data();
            int flushed = 0;
            do {
                const int cap = static_cast<int>(
                    impl->scratch.size() / (targetChannels * sizeof(float)));
                flushed = swr_convert(impl->swr, &outPtr, cap, nullptr, 0);
                if (flushed > 0) {
                    const float* fp = reinterpret_cast<const float*>(impl->scratch.data());
                    impl->pending.insert(
                        impl->pending.end(), fp,
                        fp + static_cast<std::size_t>(flushed) * targetChannels);
                }
            } while (flushed > 0);
            impl->eofDrained = true;
            return !impl->pending.empty();
        }

        if (impl->pkt->stream_index != impl->streamIndex) {
            av_packet_unref(impl->pkt);
            continue;
        }

        rc = avcodec_send_packet(impl->dec, impl->pkt);
        av_packet_unref(impl->pkt);
        if (rc < 0) {
            continue;
        }
        bool produced = false;
        while (avcodec_receive_frame(impl->dec, impl->frame) == 0) {
            convertFrame(impl, impl->frame, targetChannels, targetRate);
            av_frame_unref(impl->frame);
            produced = true;
        }
        if (produced) {
            return true;
        }
    }
}

std::size_t AudioFileDecoder::readFrames(float* out, std::size_t maxFrames) {
    if (!impl_) {
        return 0;
    }
    const std::size_t ch = static_cast<std::size_t>(targetChannels_);
    std::size_t framesOut = 0;

    while (framesOut < maxFrames) {
        if (impl_->cursor >= impl_->pending.size()) {
            impl_->pending.clear();
            impl_->cursor = 0;
            if (!decodeMore(impl_, targetChannels_, targetSampleRate_)) {
                break; // EOF and nothing left
            }
            if (impl_->pending.empty()) {
                continue;
            }
        }
        const std::size_t availFrames = (impl_->pending.size() - impl_->cursor) / ch;
        const std::size_t take = std::min(availFrames, maxFrames - framesOut);
        std::memcpy(out + framesOut * ch,
                    impl_->pending.data() + impl_->cursor,
                    take * ch * sizeof(float));
        impl_->cursor += take * ch;
        framesOut += take;
    }
    return framesOut;
}

bool AudioFileDecoder::seekToStart() {
    if (!impl_) {
        return false;
    }
    int rc = av_seek_frame(impl_->fmt, impl_->streamIndex, 0, AVSEEK_FLAG_BACKWARD);
    if (rc < 0) {
        lastError_ = "av_seek_frame: " + avErr(rc);
        return false;
    }
    avcodec_flush_buffers(impl_->dec);
    impl_->pending.clear();
    impl_->cursor = 0;
    impl_->eofDrained = false;
    return true;
}

} // namespace audio
