cmake_minimum_required(VERSION 3.22)

project(BSIIControl VERSION 0.8.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(BSII_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND BSII_FORMATS AU)
endif()

juce_add_plugin(BSIIControl
    COMPANY_NAME "GutzDev"
    BUNDLE_ID "com.gutzdev.bsiicontrol"
    PLUGIN_MANUFACTURER_CODE Gutz
    PLUGIN_CODE Bs2C
    FORMATS ${BSII_FORMATS}
    PRODUCT_NAME "BSII Control"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(BSIIControl)

target_sources(BSIIControl PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/BSIIMidi.h
    Source/BSIIPatchDecoder.h
)

target_compile_features(BSIIControl PRIVATE cxx_std_20)

target_compile_definitions(BSIIControl PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BSIIControl PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
