#include "pal/IPathService.hpp"

#include <cstdlib>

namespace pal {
namespace {

std::filesystem::path homeDir() {
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home);
    }
    return std::filesystem::temp_directory_path();
}

class PathServiceMac final : public IPathService {
public:
    std::filesystem::path dataDir() const override {
        auto p = homeDir() / "Library" / "Application Support" / "AtivaStage";
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
        auto p = homeDir() / "Library" / "Logs" / "AtivaStage";
        std::filesystem::create_directories(p);
        return p;
    }
};

} // namespace

std::unique_ptr<IPathService> makePathServiceMac() {
    return std::make_unique<PathServiceMac>();
}

} // namespace pal
