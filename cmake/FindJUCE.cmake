cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FetchContent)
include (FeatureSummary)
include (FindPackageMessage)

set_package_properties ("${CMAKE_FIND_PACKAGE_NAME}" 
                        PROPERTIES 
                            URL "https://juce.com/"
                            DESCRIPTION "Cross platform framework for plugin and app development")

FetchContent_Declare (JUCE
                      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                      GIT_TAG origin/master)

set (JUCE_BUILD_EXAMPLES OFF)
set (JUCE_BUILD_EXTRAS OFF)
set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

FetchContent_MakeAvailable (JUCE)

find_package_message ("${CMAKE_FIND_PACKAGE_NAME}" 
                      "JUCE package found -- Sources downloaded"
                      "JUCE (GitHub)")

set (${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
