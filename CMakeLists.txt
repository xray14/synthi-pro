cmake_minimum_required(VERSION 3.22)
project(SynthiPro VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # static CRT: no VC++ redistributable needed

# JUCE is expected in ./JUCE (or pass -DJUCE_DIR=<path to the JUCE source tree>).
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to the JUCE source tree")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE was not found in '${JUCE_DIR}'. Put the JUCE source tree there or pass -DJUCE_DIR=<path>. See docs/BUILD.md.")
endif()
add_subdirectory("${JUCE_DIR}" JUCE-build EXCLUDE_FROM_ALL)

juce_add_plugin(SynthiPro
    PRODUCT_NAME "SYNTHI PRO"
    COMPANY_NAME "SynthiPro Project"
    BUNDLE_ID "com.synthipro.synthipro"
    PLUGIN_MANUFACTURER_CODE Sypr
    PLUGIN_CODE Syp1
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/desktop_512.png"      # detailed icon: desktop, large views
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/taskbar_64.png")    # minimal icon: taskbar, title bar, small views

target_sources(SynthiPro PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(SynthiPro PRIVATE Source)

target_compile_definitions(SynthiPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

if(MSVC)
    target_compile_options(SynthiPro PRIVATE /utf-8 /MP)
endif()

# ASIO for the standalone app: enabled automatically when the Steinberg ASIO SDK is in ./ASIOSDK
# (or pass -DASIO_SDK_DIR=<path>). Without the SDK the app still offers WASAPI and DirectSound.
set(ASIO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK" CACHE PATH "Path to the Steinberg ASIO SDK")
if(WIN32 AND EXISTS "${ASIO_SDK_DIR}/common/iasiodrv.h")
    target_compile_definitions(SynthiPro PUBLIC JUCE_ASIO=1)
    target_include_directories(SynthiPro PUBLIC "${ASIO_SDK_DIR}/common")
    message(STATUS "SYNTHI PRO: ASIO enabled (${ASIO_SDK_DIR})")
else()
    message(STATUS "SYNTHI PRO: ASIO SDK not found - building without ASIO")
endif()

target_link_libraries(SynthiPro
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Stand-alone DSP engine test (no JUCE): ctest or build target "engine_test".
add_executable(engine_test tests/engine_test.cpp)
target_include_directories(engine_test PRIVATE Source)
if(MSVC)
    target_compile_options(engine_test PRIVATE /utf-8)
endif()
enable_testing()
add_test(NAME engine_test COMMAND engine_test WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
