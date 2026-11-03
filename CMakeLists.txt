cmake_minimum_required(VERSION 3.22)
project(Driftwood VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # no VC++ redistributable needed

# --- JUCE: local copy (-DJUCE_DIR=...) or download automatically -------------
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# --- Formats -----------------------------------------------------------------
set(DW_FORMATS VST3)
if(WIN32 OR APPLE)
    list(APPEND DW_FORMATS Standalone)
endif()
# Optional VST2: needs the legacy Steinberg VST2 SDK (not public anymore)
if(DEFINED VST2_SDK_DIR)
    juce_set_vst2_sdk_path(${VST2_SDK_DIR})
    list(APPEND DW_FORMATS VST)
endif()

juce_add_plugin(Driftwood
    COMPANY_NAME "Driftwood Audio"
    PRODUCT_NAME "Driftwood"
    PLUGIN_MANUFACTURER_CODE Drfw
    PLUGIN_CODE Dw01
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    VST2_CATEGORY kPlugCategSynth
    FORMATS ${DW_FORMATS})

target_sources(Driftwood PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Driftwood PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(Driftwood PRIVATE /utf-8)
endif()

if(WIN32 OR APPLE)
    target_link_libraries(Driftwood PRIVATE juce::juce_audio_utils)
else()
    target_link_libraries(Driftwood PRIVATE juce::juce_audio_processors)
    target_compile_definitions(Driftwood PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0)
endif()

target_link_libraries(Driftwood
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Offline test renderer (developer only) -----------------------------------
option(DW_BUILD_TESTS "Build offline render test" OFF)
if(DW_BUILD_TESTS)
    juce_add_console_app(DriftwoodRenderTest PRODUCT_NAME "DriftwoodRenderTest")
    target_sources(DriftwoodRenderTest PRIVATE Tests/RenderTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(DriftwoodRenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Driftwood"
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0)
    target_link_libraries(DriftwoodRenderTest PRIVATE juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics
        PUBLIC juce::juce_recommended_config_flags)
endif()
