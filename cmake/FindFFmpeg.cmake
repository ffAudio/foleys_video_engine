cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FeatureSummary)

set_package_properties (FFmpeg PROPERTIES 
                        URL "https://www.ffmpeg.org/" 
                        DESCRIPTION "Open source library for audio and video codecs")

set (FFmpeg_FOUND FALSE)

set (FFMPEG_LIBS avutil swresample avcodec avformat swscale CACHE INTERNAL "")

option (FFMPEG_USE_SYSTEM_INSTALL "Use preinstalled ffmpeg found on system, if any" OFF)

if (FFMPEG_USE_SYSTEM_INSTALL)
    include ("${CMAKE_CURRENT_LIST_DIR}/ffmpeg/FindInstalledFFmpeg.cmake")
endif()

if (TARGET ffmpeg)
    if (NOT FFmpeg_FIND_QUIETLY)
        message (STATUS "Using system installation of FFmpeg")
    endif()
else()
    if (FFMPEG_USE_SYSTEM_INSTALL)
        if (NOT FFmpeg_FIND_QUIETLY)
            message (WARNING "System installation of FFmpeg was requested, but targets couldn't be imported. Configuring to build from source...")
        endif()
    endif()

    include ("${CMAKE_CURRENT_LIST_DIR}/ffmpeg/BuildFFmpeg.cmake")
endif()

if (NOT TARGET ffmpeg)
    if (FFmpeg_FIND_REQUIRED)   
        message (FATAL_ERROR "Error creating ffmpeg target!")
    endif()
endif()

target_compile_definitions (ffmpeg INTERFACE FOLEYS_USE_FFMPEG=1)

add_library (ffmpeg::ffmpeg ALIAS ffmpeg)

set (FFmpeg_FOUND TRUE)
