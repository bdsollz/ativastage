#include "pal/IAudioDeviceService.hpp"
#include "pal/Pal.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace pal {
namespace {

// Marco 1.1 stub: deterministic, dependency-free implementation so libaudio and
// the control layer can be built and tested headless before the real backend is
// wired. The real implementation is backed by miniaudio (enumerating WASAPI /
// CoreAudio output devices, reporting the system default, and firing the change
// callback on hot-plug); it replaces this class when real playback lands — the
// interface does not change. Cross-platform on purpose: no #ifdef here.
class AudioDeviceServiceStub final : public IAudioDeviceService {
public:
    AudioDeviceServiceStub() {
        devices_.push_back(AudioDeviceInfo{"default", "Default Output", true});
    }

    std::vector<AudioDeviceInfo> outputDevices() const override {
        return devices_;
    }

    std::optional<AudioDeviceInfo> defaultOutputDevice() const override {
        for (const auto& d : devices_) {
            if (d.isDefault) {
                return d;
            }
        }
        return std::nullopt;
    }

    bool selectOutputDevice(const std::string& id) override {
        const bool exists = std::any_of(
            devices_.begin(), devices_.end(),
            [&](const AudioDeviceInfo& d) { return d.id == id; });
        if (!exists) {
            return false;
        }
        selected_ = id;
        return true;
    }

    std::optional<std::string> selectedOutputDevice() const override {
        return selected_;
    }

    void setDeviceChangeCallback(std::function<void()> onChange) override {
        onChange_ = std::move(onChange);
    }

private:
    std::vector<AudioDeviceInfo> devices_;
    std::optional<std::string>   selected_;
    std::function<void()>        onChange_;
};

} // namespace

std::unique_ptr<IAudioDeviceService> makeAudioDeviceService() {
    return std::make_unique<AudioDeviceServiceStub>();
}

} // namespace pal
