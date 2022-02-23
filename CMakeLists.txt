cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

project (FoleysVideoEngine VERSION 0.2.0 LANGUAGES CXX OBJC C)


set (CMAKE_SUPPRESS_REGENERATION TRUE)


list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include (FoleysDefaultSettings)
include (FoleysDefaultTarget)
include (FindJUCE)
include (FindFFmpeg)

juce_add_module ("${CMAKE_CURRENT_LIST_DIR}" ALIAS_NAMESPACE Foleys)

target_compile_definitions (foleys_video_engine INTERFACE JUCE_USE_CAMERA=1 FOLEYS_CAMERA_SUPPORT=1 
														  JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_STRICT_REFCOUNTEDPOINTER=1 
														  JUCE_PLUGINHOST_AU=1 JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_LADSPA=1)

target_link_libraries (foleys_video_engine INTERFACE Foleys::foleys_ffmpeg Foleys::FoleysDefaultTarget)

option (FOLEYS_BUILD_TESTS "" OFF)

if (FOLEYS_BUILD_TESTS)
    add_subdirectory (tests)
endif()
