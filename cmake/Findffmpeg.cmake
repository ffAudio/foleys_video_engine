cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FetchContent)
include (FeatureSummary)
include (FindPackageMessage)
include(FindPackageHandleStandardArgs)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_package_properties ("${CMAKE_FIND_PACKAGE_NAME}" 
                            PROPERTIES 
                                URL "https://www.ffmpeg.org/"
                                DESCRIPTION "Audio and video codecs")
    FetchContent_Declare (ffmpeg 
                          GIT_REPOSITORY "https://github.com/mach1studios/FFmpegBuild.git"
                          GIT_TAG origin/main)

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    
    set(BUILT_FFMPEG_RELEASE "ffmpeg-master-latest-win64-lgpl-shared.zip")

    if(NOT BUILT_FFMPEG_RELEASE)
        message(FATAL_ERROR "Platform ${CMAKE_SYSTEM_PROCESSOR} on system ${CMAKE_SYSTEM_NAME} is not supported!")
    endif()

    if (NOT "${PROJECT_BINARY_DIR}/.local" IN_LIST "${CMAKE_PREFIX_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/.local")
    endif()

    FetchContent_Declare(ffmpeg
        URL  "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/${BUILT_FFMPEG_RELEASE}"
        SOURCE_DIR "${PROJECT_BINARY_DIR}/.local"  
    )

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "amd64")
        set(BUILT_FFMPEG_RELEASE "ffmpeg-master-latest-linux64-lgpl-shared.tar.xz")
    elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64)
        set(BUILT_FFMPEG_RELEASE "ffmpeg-master-latest-linuxarm64-lgpl-shared.tar.xz")
    endif()

    if(NOT BUILT_FFMPEG_RELEASE)
        message(FATAL_ERROR "Platform ${CMAKE_SYSTEM_PROCESSOR} on system ${CMAKE_SYSTEM_NAME} is not supported!")
    endif()

    if (NOT "${PROJECT_BINARY_DIR}/.local" IN_LIST "${CMAKE_PREFIX_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/.local")
    endif()

    FetchContent_Declare(ffmpeg
        URL  "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/${BUILT_FFMPEG_RELEASE}"
        SOURCE_DIR "${PROJECT_BINARY_DIR}/.local"  
    )

endif()

FetchContent_MakeAvailable (ffmpeg)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    find_package_message ("${CMAKE_FIND_PACKAGE_NAME}" 
                          "ffmpeg package found -- Sources downloaded"
                          "ffmpeg (GitHub)")

    set (${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)

else()

macro(find_component _component _header)
    find_path(${_component}_INCLUDE_DIRS "${_header}" PATH_SUFFIXES ffmpeg)
    find_library(${_component}_LIBRARY NAMES "${_component}" PATH_SUFFIXES ffmpeg)
  
    if (${_component}_LIBRARY AND ${_component}_INCLUDE_DIRS)
        set(FFmpeg_${_component}_FOUND TRUE)
        set(FFmpeg_LINK_LIBRARIES ${FFmpeg_LINK_LIBRARIES} ${${_component}_LIBRARY})
        list(APPEND FFmpeg_INCLUDE_DIRS ${${_component}_INCLUDE_DIRS}) 

        if (NOT TARGET FFmpeg::${_component})
            add_library(FFmpeg_${_component} UNKNOWN IMPORTED)
            set_target_properties(FFmpeg_${_component} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${${_component}_INCLUDE_DIRS}"
                IMPORTED_LOCATION "${${_component}_LIBRARY}"
            )
            add_library(FFmpeg::${_component} ALIAS FFmpeg_${_component})
        endif ()
    endif ()
  
    mark_as_advanced(${_component}_INCLUDE_DIRS)
    mark_as_advanced(${_component}_LIBRARY)
endmacro()

#------------------------------------------------------------------------------

# The default components
if (NOT FFmpeg_FIND_COMPONENTS)
    set(FFmpeg_FIND_COMPONENTS avcodec avfilter avformat avdevice avutil swresample swscale)
endif ()

# Traverse the user-selected components of the package and find them
set(FFmpeg_INCLUDE_DIRS)
set(FFmpeg_LINK_LIBRARIES)
foreach(_component ${FFmpeg_FIND_COMPONENTS})
    find_component(${_component} lib${_component}/${_component}.h)
endforeach()
mark_as_advanced(FFmpeg_INCLUDE_DIRS)
mark_as_advanced(FFmpeg_LINK_LIBRARIES)

# Handle findings
list(LENGTH FFmpeg_FIND_COMPONENTS FFmpeg_COMPONENTS_COUNT)
find_package_handle_standard_args(FFmpeg REQUIRED_VARS FFmpeg_COMPONENTS_COUNT HANDLE_COMPONENTS)

# Publish targets if succeeded to find the FFmpeg package and the requested components
if (FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg INTERFACE)
    set_target_properties(FFmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${FFmpeg_LINK_LIBRARIES}"
    )
    add_library(FFmpeg::FFmpeg ALIAS FFmpeg)
endif()

endif()