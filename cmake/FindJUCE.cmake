cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FeatureSummary)

set_package_properties (JUCE PROPERTIES URL "https://juce.com/"
                        DESCRIPTION "Cross platform framework for plugin and app development")

include ("${CMAKE_CURRENT_LIST_DIR}/AddCPM.cmake")

CPMAddPackage (NAME JUCE
               GITHUB_REPOSITORY juce-framework/JUCE
               GIT_TAG origin/develop
               OPTIONS "JUCE_BUILD_EXAMPLES OFF" "JUCE_BUILD_EXTRAS OFF" "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON")

set (JUCE_FOUND TRUE)