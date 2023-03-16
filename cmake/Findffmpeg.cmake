cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FetchContent)
include (FeatureSummary)
include (FindPackageMessage)

set_package_properties ("${CMAKE_FIND_PACKAGE_NAME}" 
                        PROPERTIES 
                            URL "https://www.ffmpeg.org/"
                            DESCRIPTION "Audio and video codecs")

FetchContent_Declare (ffmpeg 
                        GIT_REPOSITORY "https://github.com/ffAudio/FFmpegBuild.git"
                        GIT_TAG origin/main)

FetchContent_MakeAvailable (ffmpeg)

find_package_message ("${CMAKE_FIND_PACKAGE_NAME}" 
                      "ffmpeg package found -- Sources downloaded"
                      "ffmpeg (GitHub)")

set (${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
