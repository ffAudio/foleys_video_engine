cmake_minimum_required (VERSION 3.22 FATAL_ERROR)

@PACKAGE_INIT@

#

include ("${CMAKE_CURRENT_LIST_DIR}/FFmpegTargets.cmake")

include (FeatureSummary)
include (FindPackageMessage)

set_package_properties ("${CMAKE_FIND_PACKAGE_NAME}"
                        PROPERTIES
                            URL "https://www.ffmpeg.org/"
                            DESCRIPTION "Audio & video codecs")

find_package_message ("${CMAKE_FIND_PACKAGE_NAME}"
                      "ffmpeg package found -- installed on system"
                      "ffmpeg (system install) [${CMAKE_CURRENT_LIST_DIR}]")

check_required_components ("${CMAKE_FIND_PACKAGE_NAME}")
