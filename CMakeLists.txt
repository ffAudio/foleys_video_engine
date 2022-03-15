cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

project (FoleysVideoEngine VERSION 0.2.0 LANGUAGES CXX OBJC C)


set (CMAKE_SUPPRESS_REGENERATION TRUE)


list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include (FoleysDefaultSettings)
include (FoleysDefaultTarget)

find_package (JUCE REQUIRED)
find_package (FFmpeg REQUIRED)

juce_add_module ("${CMAKE_CURRENT_LIST_DIR}/foleys_video_engine" ALIAS_NAMESPACE Foleys)

target_compile_definitions (foleys_video_engine INTERFACE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_STRICT_REFCOUNTEDPOINTER=1 
														  JUCE_PLUGINHOST_AU=1 JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_LADSPA=1)

target_link_libraries (foleys_video_engine INTERFACE ffmpeg::ffmpeg Foleys::FoleysDefaultTarget)
