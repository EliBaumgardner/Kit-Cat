cmake_minimum_required(VERSION 3.28)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(KitCat VERSION 0.0.1)

option(KITCAT_REALTIME_SANITIZER "Build everything with -fsanitize=realtime and add KitCat_RealtimeTests; needs upstream clang 20 or later" OFF)

set(KITCAT_COPY_PLUGIN TRUE)

if(KITCAT_REALTIME_SANITIZER)
    add_compile_options(-fsanitize=realtime -fdelayed-template-parsing -Wno-delayed-template-parsing-in-cxx20)
    add_link_options(-fsanitize=realtime)
    set(KITCAT_COPY_PLUGIN FALSE)
endif()

add_subdirectory(JUCE)
add_subdirectory(flucoma-core)

set(ARSNOVAUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ArsNovaUI" CACHE PATH "Checkout of the ArsNovaUI library")
add_subdirectory(${ARSNOVAUI_PATH} ${CMAKE_CURRENT_BINARY_DIR}/ArsNovaUI)

juce_add_plugin(KitCat
        PRODUCT_NAME "Kit-Cat"
        COMPANY_NAME "ArsNova"
        COPY_PLUGIN_AFTER_BUILD ${KITCAT_COPY_PLUGIN}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        FORMATS AU VST3 Standalone
        PLUGIN_MANUFACTURER_CODE ANON
        PLUGIN_CODE KC01
)

target_compile_features(KitCat PRIVATE cxx_std_20)

target_compile_options(KitCat PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_compile_definitions(KitCat PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_definitions(KitCat PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_sources(KitCat PRIVATE
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

target_include_directories(KitCat PRIVATE Source)

target_link_libraries(KitCat
        PRIVATE
            FLUID_DECOMPOSITION
            ArsNovaUI
            juce::juce_audio_utils
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
)

juce_add_console_app(KitCatProbe PRODUCT_NAME "KitCatProbe")

target_compile_features(KitCatProbe PRIVATE cxx_std_20)

target_compile_definitions(KitCatProbe PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_sources(KitCatProbe PRIVATE
        Source/Probe/ProbeMain.cpp
        Source/Analysis/SliceAnalysis.cpp
        Source/Analysis/CorpusAnalysis.cpp
        Source/Analysis/MapLayout.cpp
)

target_include_directories(KitCatProbe PRIVATE Source)

target_link_libraries(KitCatProbe
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

add_executable(KitCat_Tests
        Tests/TestMain.cpp
        Tests/TestCorpus.cpp
        Tests/Unit/MatchingTests.cpp
        Tests/Unit/TargetAnalyserTests.cpp
        Tests/Unit/BridgeTests.cpp
        Tests/Unit/MapLayoutTests.cpp
        Tests/Integration/LiveMatchingTests.cpp
        Tests/Integration/CorpusFileTests.cpp
)

target_compile_features(KitCat_Tests PRIVATE cxx_std_20)

target_include_directories(KitCat_Tests PRIVATE Source Tests JUCE/modules ${ARSNOVAUI_PATH})

target_link_libraries(KitCat_Tests PRIVATE KitCat FLUID_DECOMPOSITION Catch2::Catch2)

catch_discover_tests(KitCat_Tests DISCOVERY_MODE PRE_TEST)

if(KITCAT_REALTIME_SANITIZER)
    add_executable(KitCat_RealtimeTests
            Tests/TestMain.cpp
            Tests/TestCorpus.cpp
            Tests/Integration/RealtimeTests.cpp
    )

    target_compile_features(KitCat_RealtimeTests PRIVATE cxx_std_20)

    target_include_directories(KitCat_RealtimeTests PRIVATE Source Tests JUCE/modules ${ARSNOVAUI_PATH})

    target_link_libraries(KitCat_RealtimeTests PRIVATE KitCat FLUID_DECOMPOSITION Catch2::Catch2)

    catch_discover_tests(KitCat_RealtimeTests DISCOVERY_MODE PRE_TEST)
endif()
