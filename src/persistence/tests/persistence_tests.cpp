#include "persistence/Database.hpp"
#include "persistence/Migrator.hpp"

#include <catch2/catch_test_macros.hpp>

using persistence::Database;
using persistence::Migrator;

TEST_CASE("fresh database starts at schema version 0", "[persistence][migrate]") {
    Database db(":memory:");
    REQUIRE(Migrator::currentVersion(db) == 0);
}

TEST_CASE("migrate brings database to latest version", "[persistence][migrate]") {
    Database db(":memory:");
    const int latest = Migrator::migrations().back().version;

    REQUIRE(Migrator::migrate(db) == latest);
    REQUIRE(Migrator::currentVersion(db) == latest);

    // Baseline table from migration 1 must exist.
    const auto name = db.queryScalar(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='app_bootstrap';");
    REQUIRE(name == "app_bootstrap");
}

TEST_CASE("migrate is idempotent", "[persistence][migrate]") {
    Database db(":memory:");
    const int first = Migrator::migrate(db);
    const int second = Migrator::migrate(db);
    REQUIRE(first == second);
}

TEST_CASE("migration list is contiguous starting at 1", "[persistence][migrate]") {
    int expected = 1;
    for (const auto& m : Migrator::migrations()) {
        REQUIRE(m.version == expected);
        ++expected;
    }
}
