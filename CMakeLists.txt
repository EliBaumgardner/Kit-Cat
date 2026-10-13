cmake_minimum_required(VERSION 3.28)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(CatKit VERSION 0.0.1)

add_subdirectory(JUCE)
add_subdirectory(flucoma-core)

juce_add_plugin(CatKit
        PRODUCT_NAME "Cat-Kit"
        COMPANY_NAME "ArsNova"
        COPY_PLUGIN_AFTER_BUILD TRUE
        IS_SYNTH TRUE
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
        Source/Analysis/SliceAnalysis.cpp
)

target_include_directories(CatKit PRIVATE Source)

target_link_libraries(CatKit
        PRIVATE
            FLUID_DECOMPOSITION
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
)

target_include_directories(CatKitProbe PRIVATE Source)

target_link_libraries(CatKitProbe
        PRIVATE
            FLUID_DECOMPOSITION
            juce::juce_audio_formats
        PUBLIC
            juce::juce_recommended_config_flags
)
