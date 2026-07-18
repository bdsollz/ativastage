# libaudio — Audio Engine.
# Fase 1, Marco 1.1:
#   * dependency-free, real-time-safe core (deck state machine, SPSC ring buffer,
#     gain ramp, deck render path) — always built, fully testable headless.
#   * FFmpeg-backed AudioFileDecoder (the producer that feeds the ring buffer) —
#     built only when FFmpeg is available (via vcpkg). Guarded so the pure core
#     never fails to build if FFmpeg is absent.
#   * miniaudio-backed AudioOutput (real playback device running the RT callback)
#     — built only when miniaudio is available. Kept out of the CI test path
#     (opening a device needs real hardware); verified via the audio_probe tool.

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

# miniaudio is provided by vcpkg (header-only; CONFIG package exports a target).
find_package(miniaudio CONFIG QUIET)
set(ATIVASTAGE_MINIAUDIO_TARGET "")
if(TARGET miniaudio::miniaudio)
    set(ATIVASTAGE_MINIAUDIO_TARGET miniaudio::miniaudio)
elseif(TARGET unofficial::miniaudio::miniaudio)
    set(ATIVASTAGE_MINIAUDIO_TARGET unofficial::miniaudio::miniaudio)
endif()

if(ATIVASTAGE_MINIAUDIO_TARGET)
    target_sources(libaudio PRIVATE src/AudioOutput.cpp)
    target_link_libraries(libaudio PRIVATE ${ATIVASTAGE_MINIAUDIO_TARGET})
    target_compile_definitions(libaudio PUBLIC ATIVASTAGE_HAVE_MINIAUDIO=1)
    # miniaudio's backends need these system libraries.
    find_package(Threads REQUIRED)
    target_link_libraries(libaudio PRIVATE Threads::Threads)
    if(APPLE)
        target_link_libraries(libaudio PRIVATE
            "-framework CoreFoundation" "-framework CoreAudio"
            "-framework AudioToolbox" "-framework AudioUnit")
    elseif(UNIX)
        target_link_libraries(libaudio PRIVATE ${CMAKE_DL_LIBS} m)
    endif()
    message(STATUS "libaudio: miniaudio found — AudioOutput enabled.")
else()
    message(STATUS "libaudio: miniaudio not found — AudioOutput disabled.")
endif()

if(ATIVASTAGE_BUILD_TESTS AND Catch2_FOUND)
    add_executable(audio_tests tests/audio_tests.cpp)
    target_link_libraries(audio_tests PRIVATE libaudio Catch2::Catch2WithMain)
    # Absolute path to the committed test fixtures (real decoded assets).
    target_compile_definitions(audio_tests PRIVATE
        AUDIO_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    add_test(NAME audio_tests COMMAND audio_tests)
endif()

# Manual audio pipeline tool (decoder -> deck -> output). Needs both libraries
# and a real output device; not registered as a ctest.
if(FFMPEG_FOUND AND ATIVASTAGE_MINIAUDIO_TARGET)
    add_executable(audio_probe tools/audio_probe.cpp)
    target_link_libraries(audio_probe PRIVATE libaudio)
    find_package(Threads REQUIRED)
    target_link_libraries(audio_probe PRIVATE Threads::Threads)
    message(STATUS "libaudio: audio_probe tool enabled.")
endif()
