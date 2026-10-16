cmake_minimum_required(VERSION 3.28)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(CatKit VERSION 0.0.1)

option(CATKIT_REALTIME_SANITIZER "Build everything with -fsanitize=realtime and add CatKit_RealtimeTests; needs upstream clang 20 or later" OFF)

set(CATKIT_COPY_PLUGIN TRUE)

if(CATKIT_REALTIME_SANITIZER)
    add_compile_options(-fsanitize=realtime -fdelayed-template-parsing -Wno-delayed-template-parsing-in-cxx20)
    add_link_options(-fsanitize=realtime)
    set(CATKIT_COPY_PLUGIN FALSE)
endif()

add_subdirectory(JUCE)
add_subdirectory(flucoma-core)

set(ARSNOVAUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ArsNovaUI" CACHE PATH "Checkout of the ArsNovaUI library")
add_subdirectory(${ARSNOVAUI_PATH} ${CMAKE_CURRENT_BINARY_DIR}/ArsNovaUI)

juce_add_plugin(CatKit
        PRODUCT_NAME "Cat-Kit"
        COMPANY_NAME "ArsNova"
        COPY_PLUGIN_AFTER_BUILD ${CATKIT_COPY_PLUGIN}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        FORMATS AU VST3 Standalone
        PLUGIN_MANUFACTURER_CODE ANON
        PLUGIN_CODE CK01
)

target_compile_features(CatKit PRIVATE cxx_std_20)

target_compile_options(CatKit PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_compile_definitions(CatKit PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_definitions(CatKit PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_sources(CatKit PRIVATE
        Source/Plugin/PluginProcessor.cpp
        Source/Plugin/PluginEditor.cpp
        Source/Plugin/CorpusLoader.cpp
        Source/Plugin/StateIdentifiers.cpp
        Source/Analysis/SliceAnalysis.cpp
        Source/Analysis/CorpusAnalysis.cpp
        Source/Analysis/MapLayout.cpp
        Source/Map/CorpusMap.cpp
        Source/UI/CorpusBar.cpp
        Source/UI/ParameterPanel.cpp
        Source/Audio/CorpusPublisher.cpp
        Source/Audio/SlicePlayer.cpp
        Source/Audio/AudioUIBridge.cpp
        Source/Audio/TargetAnalyser.cpp
        Source/Audio/TargetMatcher.cpp
)

target_include_directories(CatKit PRIVATE Source)

target_link_libraries(CatKit
        PRIVATE
            FLUID_DECOMPOSITION
            ArsNovaUI
            juce::juce_audio_utils
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
)

juce_add_console_app(CatKitProbe PRODUCT_NAME "CatKitProbe")

target_compile_features(CatKitProbe PRIVATE cxx_std_20)

target_compile_definitions(CatKitProbe PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_sources(CatKitProbe PRIVATE
        Source/Probe/ProbeMain.cpp
        Source/Analysis/SliceAnalysis.cpp
        Source/Analysis/CorpusAnalysis.cpp
        Source/Analysis/MapLayout.cpp
)

target_include_directories(CatKitProbe PRIVATE Source)

target_link_libraries(CatKitProbe
        PRIVATE
            FLUID_DECOMPOSITION
            juce::juce_audio_formats
            juce::juce_graphics
        PUBLIC
            juce::juce_recommended_config_flags
)

add_subdirectory(Catch2)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/extras)
include(Catch)
enable_testing()

add_executable(CatKit_Tests
        Tests/TestMain.cpp
        Tests/TestCorpus.cpp
        Tests/MatchingTests.cpp
        Tests/LiveMatchingTests.cpp
        Tests/TargetAnalyserTests.cpp
        Tests/BridgeTests.cpp
        Tests/CorpusFileTests.cpp
        Tests/MapLayoutTests.cpp
)

target_compile_features(CatKit_Tests PRIVATE cxx_std_20)

target_include_directories(CatKit_Tests PRIVATE Source Tests JUCE/modules ${ARSNOVAUI_PATH})

target_link_libraries(CatKit_Tests PRIVATE CatKit FLUID_DECOMPOSITION Catch2::Catch2)

catch_discover_tests(CatKit_Tests DISCOVERY_MODE PRE_TEST)

if(CATKIT_REALTIME_SANITIZER)
    add_executable(CatKit_RealtimeTests
            Tests/TestMain.cpp
            Tests/TestCorpus.cpp
            Tests/RealtimeTests.cpp
    )

    target_compile_features(CatKit_RealtimeTests PRIVATE cxx_std_20)

    target_include_directories(CatKit_RealtimeTests PRIVATE Source Tests JUCE/modules ${ARSNOVAUI_PATH})

    target_link_libraries(CatKit_RealtimeTests PRIVATE CatKit FLUID_DECOMPOSITION Catch2::Catch2)

    catch_discover_tests(CatKit_RealtimeTests DISCOVERY_MODE PRE_TEST)
endif()
