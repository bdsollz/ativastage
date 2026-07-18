#include "pal/IPathService.hpp"

#include <cstdlib>

namespace pal {
namespace {

std::filesystem::path appDataDir() {
#if defined(_WIN32)
    // %APPDATA% is set for interactive sessions; fall back to a temp path.
    size_t len = 0;
    char buf[32767];
    if (getenv_s(&len, buf, sizeof(buf), "APPDATA") == 0 && len > 0) {
        return std::filesystem::path(buf);
    }
#endif
    return std::filesystem::temp_directory_path();
}

class PathServiceWin final : public IPathService {
public:
    std::filesystem::path dataDir() const override {
        auto p = appDataDir() / "AtivaStage";
        std::filesystem::create_directories(p);
        return p;
    }
    std::filesystem::path mediaDir() const override {
        auto p = dataDir() / "media";
        std::filesystem::create_directories(p);
        return p;
    }
    std::filesystem::path backupsDir() const override {
        auto p = dataDir() / "backups";
        std::filesystem::create_directories(p);
        return p;
    }
    std::filesystem::path logsDir() const override {
        auto p = dataDir() / "logs";
        std::filesystem::create_directories(p);
        return p;
    }
};

} // namespace

std::unique_ptr<IPathService> makePathServiceWin() {
    return std::make_unique<PathServiceWin>();
}

} // namespace pal
