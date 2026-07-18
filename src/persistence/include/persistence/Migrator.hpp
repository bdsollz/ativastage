#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace persistence {

class Database;

// A single, numbered, forward-only migration. `version` must be strictly
// increasing and contiguous starting at 1.
struct Migration {
    int version;
    std::string name;
    std::string sql; // applied inside a transaction
};

// Applies pending migrations to bring the database to the latest version.
// Bootstraps the `meta` table and tracks `schema_version` there. Each
// migration runs in its own transaction; a failure rolls back and throws,
// leaving `schema_version` at the last good value (Seção 4.6).
class Migrator {
public:
    // Returns the full ordered list of migrations known to the application.
    static const std::vector<Migration>& migrations();

    // Ensures the meta table exists and returns the current schema version
    // (0 for a fresh database).
    static int currentVersion(Database& db);

    // Applies all pending migrations. Returns the resulting schema version.
    static int migrate(Database& db);
};

} // namespace persistence
