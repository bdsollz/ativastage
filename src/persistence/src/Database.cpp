#include "persistence/Database.hpp"

#include <sqlite3.h>

#include <utility>

namespace persistence {

Database::Database(const std::filesystem::path& path) {
    const std::string p = path.string();
    if (sqlite3_open(p.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown error";
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseError("failed to open database '" + p + "': " + msg);
    }
    // Offline-first pragmas (Seção 4.6). WAL is a no-op for :memory: but harmless.
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA foreign_keys=ON;");
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

Database::Database(Database&& other) noexcept : db_(std::exchange(other.db_, nullptr)) {}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = std::exchange(other.db_, nullptr);
    }
    return *this;
}

void Database::exec(std::string_view sql) {
    char* err = nullptr;
    const std::string stmt(sql);
    if (sqlite3_exec(db_, stmt.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw DatabaseError("SQL error: " + msg);
    }
}

std::string Database::queryScalar(std::string_view sql, std::string fallback) {
    sqlite3_stmt* stmt = nullptr;
    const std::string q(sql);
    if (sqlite3_prepare_v2(db_, q.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        throw DatabaseError("prepare failed: " + msg);
    }
    std::string result = std::move(fallback);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (const unsigned char* text = sqlite3_column_text(stmt, 0)) {
            result = reinterpret_cast<const char*>(text);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace persistence
