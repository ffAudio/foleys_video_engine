cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(foleys_video_engine VERSION 0.0.1)

set(BaseTargetName foleys_video_engine)

set(ProductName "foleys_video_engine" "Set this to the name of the plugin")
set(CompanyName "Foleys Finest Audio" "Set this to the name of the manufacturer")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeIncludes")

include(System)

#We 'require' that we need juce. If JUCE isn't found, it will revert to what's in
#CMake/Findjuce.cmake, where you can see how JUCE is brought in/configured
find_package(juce REQUIRED)

add_subdirectory(modules)

add_subdirectory(examples)
