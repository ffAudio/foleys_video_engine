cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

find_program (FFMPEG_MAKE_EXECUTABLE NAMES make gmake nmake
              DOC "Path to the make executable used for building FFmpeg")

mark_as_advanced (FORCE FFMPEG_MAKE_EXECUTABLE)

if (NOT FFMPEG_MAKE_EXECUTABLE)
    message (FATAL_ERROR "Make could not be found, and is required to build FFmpeg!")
endif()

set (FFMPEG_CONFIGURE_EXTRAS "" CACHE STRING "Extra flags passed to FFmpeg's configuration script")
set (FFMPEG_EXTRA_C_FLAGS "" CACHE STRING "Extra C flags for FFmpeg")
set (FFMPEG_EXTRA_LD_FLAGS "" CACHE STRING "Extra linker flags for FFmpeg")
set (FFMPEG_VERSION 4.4.1 CACHE STRING "FFmpeg version")

mark_as_advanced (FORCE FFMPEG_CONFIGURE_EXTRAS FFMPEG_EXTRA_C_FLAGS FFMPEG_EXTRA_LD_FLAGS FFMPEG_VERSION)

set (FFMPEG_NAME "ffmpeg-${FFMPEG_VERSION}")

if (DEFINED ENV{CPM_SOURCE_CACHE})
    set (FOLEYS_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}")
else()
    set (FOLEYS_SOURCE_CACHE "${PROJECT_SOURCE_DIR}/Cache")
endif()

set (FFMPEG_SOURCE_DIR "${FOLEYS_SOURCE_CACHE}/${FFMPEG_NAME}")

#

if (NOT EXISTS "${FFMPEG_SOURCE_DIR}")

    set (FFMPEG_URL "https://ffmpeg.org/releases/${FFMPEG_NAME}.tar.bz2")

    if (NOT FFmpeg_FIND_QUIETLY)
        message (STATUS "Downloading FFmpeg sources from ${FFMPEG_URL} to ${FFMPEG_SOURCE_DIR}...")
    endif()

    get_filename_component (FFMPEG_ARCHIVE_NAME "${FFMPEG_URL}" NAME)

    set (ffmpeg_tarball "${FOLEYS_SOURCE_CACHE}/${FFMPEG_ARCHIVE_NAME}")

    file (DOWNLOAD "${FFMPEG_URL}" "${ffmpeg_tarball}")

    execute_process (COMMAND "${CMAKE_COMMAND}" -E tar xzf "${ffmpeg_tarball}"
                     WORKING_DIRECTORY "${FOLEYS_SOURCE_CACHE}")

    file (REMOVE "${ffmpeg_tarball}")

endif()

#

string (REPLACE " -Wl,--fatal-warnings" "" FFMPEG_LD_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set (FFMPEG_LD_FLAGS "${FFMPEG_C_FLAGS} ${FFMPEG_LD_FLAGS} ${FFMPEG_EXTRA_LD_FLAGS}")

#

include ("${CMAKE_CURRENT_LIST_DIR}/ConfigureFFmpegBuild.cmake")

foleys_configure_ffmpeg_build (SOURCE_DIR "${FFMPEG_SOURCE_DIR}")


