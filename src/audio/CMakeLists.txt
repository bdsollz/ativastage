# libaudio — Audio Engine.
# Fase 1, Marco 1.1: the dependency-free, real-time-safe core (deck state
# machine, SPSC ring buffer, gain ramp, deck render path). The device backend
# (miniaudio, via pal::IAudioDeviceService) and the FFmpeg decoder that feeds
# the ring buffer are wired in a later step; the core here is fully testable
# headless and carries no external dependency yet.

add_library(libaudio STATIC
    src/AudioDeck.cpp
)

target_include_directories(libaudio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(libaudio PROPERTIES OUTPUT_NAME audio)

if(ATIVASTAGE_BUILD_TESTS AND Catch2_FOUND)
    add_executable(audio_tests tests/audio_tests.cpp)
    target_link_libraries(audio_tests PRIVATE libaudio Catch2::Catch2WithMain)
    add_test(NAME audio_tests COMMAND audio_tests)
endif()
