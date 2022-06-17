#[[

CMake package configuration file for foleys_video_engine

This file is loaded by the find_package command to load an installed copy of foleys_video_engine
and bring it into another CMake build tree.

The way our install works is that we replicate the layout of the source tree in the package root, and 
in this file we call juce_add_module just the same way that building this project does.

]]

@PACKAGE_INIT@

check_required_components ("@PROJECT_NAME@")

#

include (FeatureSummary)
include (FindPackageMessage)

set_package_properties ("${CMAKE_FIND_PACKAGE_NAME}" 
						PROPERTIES 
							URL "https://foleysfinest.com/foleys_video_engine/"
                        	DESCRIPTION "A video engine module for JUCE")

#

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include (CMakeFindDependencyMacro)

find_dependency (JUCE 6)
find_dependency (FFmpeg)

juce_add_module ("${CMAKE_CURRENT_LIST_DIR}"
                 ALIAS_NAMESPACE Foleys)

target_compile_definitions (foleys_video_engine INTERFACE 
								JUCE_MODAL_LOOPS_PERMITTED=1 
								JUCE_STRICT_REFCOUNTEDPOINTER=1 
								JUCE_PLUGINHOST_AU=1 
								JUCE_PLUGINHOST_VST3=1 
								JUCE_PLUGINHOST_LADSPA=1)

target_link_libraries (foleys_video_engine INTERFACE ffmpeg::ffmpeg)

find_package_message ("${CMAKE_FIND_PACKAGE_NAME}" 
					  "foleys_video_engine - local install"
                      "foleys_video_engine [${CMAKE_CURRENT_LIST_DIR}]")

