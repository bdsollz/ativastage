#include "pal/IScreenService.hpp"

#include <memory>

namespace pal {
namespace {

// Fase 0 stub: real enumeration will be wired to QGuiApplication::screens()
// once libpal is allowed to depend on Qt Gui in the app composition layer.
// Kept deterministic so tests can rely on it.
class ScreenServiceMac final : public IScreenService {
public:
    std::vector<ScreenInfo> screens() const override {
        return { primaryScreen() };
    }
    ScreenInfo primaryScreen() const override {
        ScreenInfo s;
        s.id = "mac-primary";
        s.name = "Built-in Display";
        s.width = 1920;
        s.height = 1080;
        s.devicePixelRatio = 2.0;
        s.primary = true;
        return s;
    }
};

} // namespace

std::unique_ptr<IScreenService> makeScreenServiceMac() {
    return std::make_unique<ScreenServiceMac>();
}

} // namespace pal
