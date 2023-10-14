cmake_minimum_required(VERSION 3.12)

project(MULTI_CAM_TEST VERSION 0.0.1)
set(CMAKE_BINARY_DIR  build)

set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules" CACHE INTERNAL
    "The path to JUCE modules")

#find_package(JUCE CONFIG REQUIRED)
add_subdirectory(${JUCE_SOURCE_DIR} JUCE)

option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" OFF)

juce_add_console_app(MultiCamTest
    PRODUCT_NAME "Multi Camera Test")

juce_generate_juce_header(MultiCamTest)

target_sources(MultiCamTest
    PRIVATE
        Main.cpp)

target_link_libraries(MultiCamTest
    PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        foleys::foleys_video_engine
        foleys::foleys_video_addons
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
