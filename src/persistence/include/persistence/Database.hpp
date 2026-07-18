#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

struct sqlite3;

namespace persistence {

class DatabaseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Thin RAII wrapper over a SQLite connection. WAL mode is enabled on open
// (Seção 4.6). Not thread-safe: the app uses a single writer on the main
// thread (single-writer principle, Seção 2.3).
class Database {
public:
    // Opens (creating if needed) the database at `path`. Use ":memory:" for
    // an in-memory database (tests).
    explicit Database(const std::filesystem::path& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) noexcept;
    Database& operator=(Database&&) noexcept;

    // Executes one or more SQL statements with no result rows. Throws
    // DatabaseError on failure.
    void exec(std::string_view sql);

    // Convenience: reads a single string value from a scalar query. Returns
    // `fallback` if the query yields no row.
    std::string queryScalar(std::string_view sql, std::string fallback = {});

    sqlite3* handle() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

} // namespace persistence
