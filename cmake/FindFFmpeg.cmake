cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

set (foleys_ffmpeg_libs avutil swresample avcodec avformat swscale CACHE INTERNAL "")

option (FOLEYS_USE_SYSTEM_FFMPEG "Use preinstalled ffmpeg found on system, if any" OFF)

if (FOLEYS_USE_SYSTEM_FFMPEG)
    include ("${CMAKE_CURRENT_LIST_DIR}/ffmpeg/FindInstalledFFmpeg.cmake")
endif()

if (NOT TARGET foleys_ffmpeg)
    if (FOLEYS_USE_SYSTEM_FFMPEG)
        message (WARNING "System installation of FFmpeg was requested, but targets couldn't be imported. Configuring to build from source...")
    endif()

    include ("${CMAKE_CURRENT_LIST_DIR}/ffmpeg/BuildFFmpeg.cmake")
endif()

if (NOT TARGET foleys_ffmpeg)
    message (FATAL_ERROR "Error creating foleys_ffmpeg target!")
endif()

# customXcodeResourceFolders

target_compile_definitions (foleys_ffmpeg INTERFACE FOLEYS_USE_FFMPEG=1)

add_library (Foleys::foleys_ffmpeg ALIAS foleys_ffmpeg)
