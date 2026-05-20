cmake_minimum_required(VERSION 3.22)
project(RhythmPanner VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(RhythmPanner
    COMPANY_NAME "DIY"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Diyy
    PLUGIN_CODE Rpan
    FORMATS VST3
    PRODUCT_NAME "RhythmPanner"
    VST3_CATEGORIES "Fx" "Spatial"
)

juce_generate_juce_header(RhythmPanner)

target_sources(RhythmPanner PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
)

target_compile_definitions(RhythmPanner PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RhythmPanner PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
