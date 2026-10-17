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

option(BSII_BUILD_TESTS "Build hardware-capture regression tests" OFF)
if(BSII_BUILD_TESTS)
    enable_testing()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(NAME MidiLabTools COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/MidiLabTest.py")
    set_tests_properties(MidiLabTools PROPERTIES ENVIRONMENT "BSII_PATCH_INSPECT=$<TARGET_FILE:BSIIPatchInspect>")

    add_executable(BSIIPatchInspect tools/midi-lab/PatchInspect.cpp)
    target_include_directories(BSIIPatchInspect PRIVATE tests Source)
    target_compile_features(BSIIPatchInspect PRIVATE cxx_std_20)
    target_compile_definitions(BSIIPatchInspect PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BSIIPatchInspect PRIVATE juce::juce_core)

    # Explicit native-port integration check; not part of offline CTest.
    add_executable(BSIIDirectSysExTest tests/DirectSysExTest.cpp)
    target_include_directories(BSIIDirectSysExTest PRIVATE Source)
    target_compile_features(BSIIDirectSysExTest PRIVATE cxx_std_20)
    target_compile_definitions(BSIIDirectSysExTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(BSIIDirectSysExTest PRIVATE juce::juce_audio_devices)

    if(APPLE)
        # Real processor + separate-process virtual MIDI sink. No physical synth I/O.
        add_executable(BSIIProcessorAutomationTest tests/ProcessorAutomationTest.cpp)
        target_include_directories(BSIIProcessorAutomationTest PRIVATE Source
            "${CMAKE_CURRENT_BINARY_DIR}/BSIIControl_artefacts/JuceLibraryCode"
            $<TARGET_PROPERTY:BSIIControl,INCLUDE_DIRECTORIES>)
        target_compile_features(BSIIProcessorAutomationTest PRIVATE cxx_std_20)
        target_compile_definitions(BSIIProcessorAutomationTest PRIVATE
            $<TARGET_PROPERTY:BSIIControl,COMPILE_DEFINITIONS>)
        target_link_libraries(BSIIProcessorAutomationTest PRIVATE BSIIControl)

    endif()

    add_executable(BSIIMidiRoutingTest tests/MidiRoutingTest.cpp)
    target_include_directories(BSIIMidiRoutingTest PRIVATE Source)
    target_compile_features(BSIIMidiRoutingTest PRIVATE cxx_std_20)
    target_compile_definitions(BSIIMidiRoutingTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BSIIMidiRoutingTest PRIVATE juce::juce_audio_basics)
    add_test(NAME StandaloneMidiRouting COMMAND BSIIMidiRoutingTest)
    add_executable(BSIISnapshotRecallTest tests/SnapshotRecallTest.cpp)
    target_include_directories(BSIISnapshotRecallTest PRIVATE tests Source)
    target_compile_features(BSIISnapshotRecallTest PRIVATE cxx_std_20)
    target_compile_definitions(BSIISnapshotRecallTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BSIISnapshotRecallTest PRIVATE juce::juce_core)
    foreach(state baseline alternate)
        add_test(NAME SnapshotRoundtrip_${state} COMMAND BSIISnapshotRecallTest snapshot
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/${state}.bsiistate"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/${state}-roundtrip.bsiistate")
    endforeach()
    add_test(NAME SendAllBaselineRecall COMMAND BSIISnapshotRecallTest patch
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/baseline.bsiistate"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/baseline.syx")
    add_test(NAME SendAllAlternateRecall COMMAND BSIISnapshotRecallTest patch
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/alternate.bsiistate"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/state-fixtures/alternate-recalled.syx")
    add_executable(BSIIOsc1RangeTest tests/Osc1RangeTest.cpp)
    target_include_directories(BSIIOsc1RangeTest PRIVATE tests Source)
    target_compile_features(BSIIOsc1RangeTest PRIVATE cxx_std_20)
    target_compile_definitions(BSIIOsc1RangeTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(BSIIOsc1RangeTest PRIVATE juce::juce_core)
    add_test(NAME Osc1RangeCapturedPatches COMMAND BSIIOsc1RangeTest "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
endif()
