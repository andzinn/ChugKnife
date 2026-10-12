cmake_minimum_required(VERSION 3.22)

project(ChugKnife VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime for Windows release builds to avoid missing VC++ redistributable dll errors
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Try finding JUCE locally or fetch it via FetchContent
find_package(JUCE CONFIG QUIET)

if (NOT JUCE_FOUND)
    include(FetchContent)
    message(STATUS "JUCE not found locally. Fetching JUCE via FetchContent...")
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.12
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Define the audio plugin target
juce_add_plugin(ChugKnife
    COMPANY_NAME "HeavyAudio"
    PLUGIN_MANUFACTURER_CODE HvyA
    PLUGIN_CODE ChgK
    FORMATS VST3 Standalone
    PRODUCT_NAME "ChugKnife"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(ChugKnife PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/dsp/NoiseGate.h
    Source/dsp/NoiseGate.cpp
    Source/dsp/PitchShifter.h
    Source/dsp/PitchShifter.cpp
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
    Source/dsp/Chugger.h
    Source/dsp/Chugger.cpp
)

target_include_directories(ChugKnife PRIVATE
    Source
    Source/dsp
)

target_compile_definitions(ChugKnife PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ChugKnife PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
    juce::juce_data_structures
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Automated DSP and Routing Test Harness Target
add_executable(ChugKnifeTests
    Source/tests/ChugKnifeTests.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/dsp/NoiseGate.h
    Source/dsp/NoiseGate.cpp
    Source/dsp/PitchShifter.h
    Source/dsp/PitchShifter.cpp
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
    Source/dsp/Chugger.h
    Source/dsp/Chugger.cpp
)

target_include_directories(ChugKnifeTests PRIVATE
    Source
    Source/dsp
)

target_compile_definitions(ChugKnifeTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=0
)

target_link_libraries(ChugKnifeTests PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
    juce::juce_data_structures
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

