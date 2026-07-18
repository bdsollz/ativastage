#include "pal/IScreenService.hpp"

#include <memory>

namespace pal {
namespace {

// Fase 0 stub: real enumeration (EnumDisplayMonitors / QScreen) wired later.
class ScreenServiceWin final : public IScreenService {
public:
    std::vector<ScreenInfo> screens() const override {
        return { primaryScreen() };
    }
    ScreenInfo primaryScreen() const override {
        ScreenInfo s;
        s.id = "win-primary";
        s.name = "Primary Monitor";
        s.width = 1920;
        s.height = 1080;
        s.devicePixelRatio = 1.0;
        s.primary = true;
        return s;
    }
};

} // namespace

std::unique_ptr<IScreenService> makeScreenServiceWin() {
    return std::make_unique<ScreenServiceWin>();
}

} // namespace pal
