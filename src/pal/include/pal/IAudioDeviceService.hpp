#pragma once

#include "pal/Types.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace pal {

// Enumeration and selection of audio OUTPUT devices, plus hot-swap
// notification (device removed / default changed). Backed by miniaudio in the
// real implementation — miniaudio is cross-platform, so there is intentionally
// no per-platform file here (WASAPI/CoreAudio are abstracted by the backend,
// not by #ifdef). This is the platform boundary for audio devices (Seção 2.4).
//
// Thread note: enumeration/selection are called from the control thread. The
// removal callback may fire from a backend thread; implementations must marshal
// or keep the callback trivial. The real-time audio callback itself lives in
// libaudio, never here.
class IAudioDeviceService {
public:
    virtual ~IAudioDeviceService() = default;

    // Currently connected output devices. May be empty (no audio hardware).
    virtual std::vector<AudioDeviceInfo> outputDevices() const = 0;

    // System default output device, if any.
    virtual std::optional<AudioDeviceInfo> defaultOutputDevice() const = 0;

    // Selects the active output device by id. Returns false if the id is not
    // among the currently connected devices.
    virtual bool selectOutputDevice(const std::string& id) = 0;

    // Id of the currently selected output device, if one is selected.
    virtual std::optional<std::string> selectedOutputDevice() const = 0;

    // Registers a callback invoked when the device set changes (hot-plug /
    // removal of the active device / default change). Passing an empty function
    // clears the handler.
    virtual void setDeviceChangeCallback(std::function<void()> onChange) = 0;
};

} // namespace pal
