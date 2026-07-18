#pragma once

#include <cstdint>
#include <string>

namespace pal {

// Basic screen descriptor. Kept free of Qt so libpal stays UI-agnostic in
// Fase 0. Richer enumeration (via QGuiApplication::screens) is wired later.
struct ScreenInfo {
    std::string id;        // stable, persistent identifier (e.g. "HDMI-2")
    std::string name;      // human-readable name
    int32_t x = 0;         // virtual desktop position
    int32_t y = 0;
    int32_t width = 0;     // pixels
    int32_t height = 0;
    double devicePixelRatio = 1.0;
    bool primary = false;
};

} // namespace pal
