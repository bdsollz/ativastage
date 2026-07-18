find_package(unofficial-sqlite3 CONFIG QUIET)

add_library(libpersistence STATIC
    src/Database.cpp
    src/Migrator.cpp
)

target_include_directories(libpersistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(libpersistence PROPERTIES OUTPUT_NAME persistence)

# vcpkg exposes SQLite as unofficial::sqlite3; fall back to a plain find if a
# system SQLite is used instead.
if(TARGET unofficial::sqlite3::sqlite3)
    target_link_libraries(libpersistence PUBLIC unofficial::sqlite3::sqlite3)
else()
    find_package(SQLite3 REQUIRED)
    # CMake's FindSQLite3 exposes SQLite3::SQLite3 (o antigo SQLite::SQLite3
    # está depreciado).
    target_link_libraries(libpersistence PUBLIC SQLite3::SQLite3)
endif()

if(ATIVASTAGE_BUILD_TESTS AND Catch2_FOUND)
    add_executable(persistence_tests tests/persistence_tests.cpp)
    target_link_libraries(persistence_tests PRIVATE libpersistence Catch2::Catch2WithMain)
    add_test(NAME persistence_tests COMMAND persistence_tests)
endif()
