add_library(libpal STATIC
    src/Pal.cpp
)

target_include_directories(libpal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    target_sources(libpal PRIVATE
        src/win/PathService_win.cpp
        src/win/ScreenService_win.cpp
    )
else()
    target_sources(libpal PRIVATE
        src/mac/PathService_mac.cpp
        src/mac/ScreenService_mac.cpp
    )
endif()

set_target_properties(libpal PROPERTIES OUTPUT_NAME pal)

if(ATIVASTAGE_BUILD_TESTS AND Catch2_FOUND)
    add_executable(pal_tests tests/pal_tests.cpp)
    target_link_libraries(pal_tests PRIVATE libpal Catch2::Catch2WithMain)
    add_test(NAME pal_tests COMMAND pal_tests)
endif()
