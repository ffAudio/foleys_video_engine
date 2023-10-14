cmake_minimum_required(VERSION 3.17)

set(PROJECT_NAME VideoPlayer)

set(PROJECT_SOURCES
        Sources/main.cpp
        Sources/mainwindow.cpp
        Sources/mainwindow.h
        Sources/mainwindow.ui
        Sources/AudioPluginManager.cpp
        Sources/AudioPluginManager.h
)

project("${PROJECT_NAME}" LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Configure JUCE sources
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)

# The source tree should uhhh, still look like the source tree, yo
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "" FILES ${SourceFiles})


add_subdirectory(JUCE)

juce_add_gui_app("${PROJECT_NAME}"
    PRODUCT_NAME "${PROJECT_NAME}"
    VERSION 1.0.0)

juce_add_modules(
    ../modules/)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_audio_processors
    foleys::foleys_video_engine
    foleys::foleys_video_addons
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
