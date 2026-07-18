# libaudio — Audio Engine.
# Fase 1, Marco 1.1:
#   * dependency-free, real-time-safe core (deck state machine, SPSC ring buffer,
#     gain ramp, deck render path) — always built, fully testable headless.
#   * FFmpeg-backed AudioFileDecoder (the producer that feeds the ring buffer) —
#     built only when FFmpeg is available (via vcpkg). Guarded so the pure core
#     never fails to build if FFmpeg is absent.
# The miniaudio device backend (pal::IAudioDeviceService) is wired in a later step.

add_library(libaudio STATIC
    src/AudioDeck.cpp
)

target_include_directories(libaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(libaudio PROPERTIES OUTPUT_NAME audio)

# FFmpeg is provided by vcpkg (ffmpeg port ships FindFFMPEG.cmake).
find_package(FFMPEG QUIET)
if(FFMPEG_FOUND)
    target_sources(libaudio PRIVATE src/AudioFileDecoder.cpp)
    target_include_directories(libaudio PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(libaudio PRIVATE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(libaudio PRIVATE ${FFMPEG_LIBRARIES})
    target_compile_definitions(libaudio PUBLIC ATIVASTAGE_HAVE_FFMPEG=1)
    message(STATUS "libaudio: FFmpeg found — AudioFileDecoder enabled.")
else()
    message(STATUS "libaudio: FFmpeg not found — building pure core only "
                   "(AudioFileDecoder disabled).")
endif()

if(ATIVASTAGE_BUILD_TESTS AND Catch2_FOUND)
    add_executable(audio_tests tests/audio_tests.cpp)
    target_link_libraries(audio_tests PRIVATE libaudio Catch2::Catch2WithMain)
    # Absolute path to the committed test fixtures (real decoded assets).
    target_compile_definitions(audio_tests PRIVATE
        AUDIO_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME audio_tests COMMAND audio_tests)
endif()
