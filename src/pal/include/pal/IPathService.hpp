#pragma once

#include <filesystem>

namespace pal {

// Platform-specific application directories.
//   macOS   -> ~/Library/Application Support/AtivaStage/...
//   Windows -> %APPDATA%/AtivaStage/...
// Directories are created on demand by the implementation.
class IPathService {
public:
    virtual ~IPathService() = default;

    // Root data directory for the application.
    virtual std::filesystem::path dataDir() const = 0;
    // Where imported/managed media lives.
    virtual std::filesystem::path mediaDir() const = 0;
    // Rotating database backups.
    virtual std::filesystem::path backupsDir() const = 0;
    // Log files.
    virtual std::filesystem::path logsDir() const = 0;
};

} // namespace pal
