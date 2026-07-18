#include "persistence/Migrator.hpp"

#include "persistence/Database.hpp"

#include <stdexcept>

namespace persistence {

namespace {

// Ordered, forward-only migration list. Add new migrations by appending with
// the next contiguous version number; never edit or reorder existing ones
// (Seção 4.6 / diretriz 8 do plano: schema muda por migração + ADR).
const std::vector<Migration> kMigrations = {
    Migration{
        1,
        "baseline",
        // Fase 0 baseline: proves the migration engine works end to end.
        // Real content schema (songs, bible, services, ...) arrives in later
        // phases, each as its own migration with an accompanying ADR.
        "CREATE TABLE IF NOT EXISTS app_bootstrap ("
        "  id INTEGER PRIMARY KEY,"
        "  created_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"},
};

void ensureMeta(Database& db) {
    db.exec(
        "CREATE TABLE IF NOT EXISTS meta ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");");
    db.exec(
        "INSERT OR IGNORE INTO meta(key, value) VALUES ('schema_version', '0');");
}

void setVersion(Database& db, int version) {
    db.exec("UPDATE meta SET value = '" + std::to_string(version) +
            "' WHERE key = 'schema_version';");
}

} // namespace

const std::vector<Migration>& Migrator::migrations() {
    return kMigrations;
}

int Migrator::currentVersion(Database& db) {
    ensureMeta(db);
    return std::stoi(db.queryScalar(
        "SELECT value FROM meta WHERE key = 'schema_version';", "0"));
}

int Migrator::migrate(Database& db) {
    int version = currentVersion(db);

    for (const auto& m : kMigrations) {
        if (m.version <= version) {
            continue;
        }
        if (m.version != version + 1) {
            throw DatabaseError(
                "non-contiguous migration: expected " +
                std::to_string(version + 1) + ", found " +
                std::to_string(m.version));
        }
        db.exec("BEGIN;");
        try {
            db.exec(m.sql);
            setVersion(db, m.version);
            db.exec("COMMIT;");
        } catch (...) {
            db.exec("ROLLBACK;");
            throw;
        }
        version = m.version;
    }
    return version;
}

} // namespace persistence
