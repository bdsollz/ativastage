#include "pal/Pal.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("PathService returns existing, distinct directories", "[pal][path]") {
    auto paths = pal::makePathService();
    REQUIRE(paths != nullptr);

    const auto data    = paths->dataDir();
    const auto media   = paths->mediaDir();
    const auto backups = paths->backupsDir();
    const auto logs    = paths->logsDir();

    REQUIRE(std::filesystem::exists(data));
    REQUIRE(std::filesystem::exists(media));
    REQUIRE(std::filesystem::exists(backups));
    REQUIRE(std::filesystem::exists(logs));

    REQUIRE(media != data);
    REQUIRE(backups != data);
    REQUIRE(backups != media);
}

TEST_CASE("ScreenService reports at least one primary screen", "[pal][screen]") {
    auto screens = pal::makeScreenService();
    REQUIRE(screens != nullptr);

    const auto all = screens->screens();
    REQUIRE_FALSE(all.empty());

    const auto primary = screens->primaryScreen();
    REQUIRE(primary.primary);
    REQUIRE(primary.width > 0);
    REQUIRE(primary.height > 0);
    REQUIRE_FALSE(primary.id.empty());
}
