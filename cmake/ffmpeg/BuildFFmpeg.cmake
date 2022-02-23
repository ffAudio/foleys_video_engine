cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

set (FFMPEG_CONFIGURE_EXTRAS "")
set (FFMPEG_EXTRA_C_FLAGS "")
set (FFMPEG_EXTRA_LD_FLAGS "")

#

set (FFMPEG_VERSION 4.4.1)
set (FFMPEG_NAME "ffmpeg-${FFMPEG_VERSION}")
set (FFMPEG_URL "https://ffmpeg.org/releases/${FFMPEG_NAME}.tar.bz2")

if (DEFINED ENV{CPM_SOURCE_CACHE})
    set (FOLEYS_SOURCE_CACHE "$ENV{CPM_SOURCE_CACHE}")
else()
    set (FOLEYS_SOURCE_CACHE "${PROJECT_SOURCE_DIR}/Cache")
endif()

set (FFMPEG_SOURCE_DIR "${FOLEYS_SOURCE_CACHE}/${FFMPEG_NAME}")

set (FFMPEG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")

#

if (NOT EXISTS "${FFMPEG_SOURCE_DIR}")

    message (STATUS "Downloading FFmpeg sources from ${FFMPEG_URL} to ${FFMPEG_SOURCE_DIR}...")

    get_filename_component (FFMPEG_ARCHIVE_NAME "${FFMPEG_URL}" NAME)

    set (ffmpeg_tarball "${FOLEYS_SOURCE_CACHE}/${FFMPEG_ARCHIVE_NAME}")

    file (DOWNLOAD "${FFMPEG_URL}" "${ffmpeg_tarball}")

    execute_process (COMMAND ${CMAKE_COMMAND} -E tar xzf "${ffmpeg_tarball}"
                     WORKING_DIRECTORY "${FOLEYS_SOURCE_CACHE}")

    file (REMOVE "${ffmpeg_tarball}")

endif()

#

set (FFMPEG_CC "${CMAKE_C_COMPILER}")

#set (FFMPEG_C_FLAGS "${CMAKE_C_FLAGS} --target=${ANDROID_LLVM_TRIPLE} --gcc-toolchain=${ANDROID_TOOLCHAIN_ROOT} ${FFMPEG_EXTRA_C_FLAGS}")

string (REPLACE " -Wl,--fatal-warnings" "" FFMPEG_LD_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set (FFMPEG_LD_FLAGS "${FFMPEG_C_FLAGS} ${FFMPEG_LD_FLAGS} ${FFMPEG_EXTRA_LD_FLAGS}")

#set (FFMPEG_AR "${ANDROID_AR}")
#set (FFMPEG_AS "${ANDROID_ASM_COMPILER}")
#set (FFMPEG_RANLIB "${ANDROID_TOOLCHAIN_PREFIX}ranlib${ANDROID_TOOLCHAIN_SUFFIX}")
#set (FFMPEG_STRIP "${ANDROID_TOOLCHAIN_ROOT}/bin/llvm-strip${ANDROID_TOOLCHAIN_SUFFIX}")
#set (FFMPEG_NM "${ANDROID_TOOLCHAIN_PREFIX}nm${ANDROID_TOOLCHAIN_SUFFIX}")

#set (HOST_BIN "${ANDROID_NDK}/prebuilt/${ANDROID_HOST_TAG}/bin")

if ("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL x86)
    list (APPEND FFMPEG_CONFIGURE_EXTRAS --disable-asm)
endif()

string (REPLACE ";" "|" FFMPEG_CONFIGURE_EXTRAS_ENCODED "${FFMPEG_CONFIGURE_EXTRAS}")

#

#
# CMAKE_SYSTEM_PROCESSOR

## FFMPEG_ASM_FLAGS ?

include (ExternalProject)

configure_file ("${CMAKE_CURRENT_LIST_DIR}/configure_ffmpeg_build.cmake" configure_ffmpeg_build.cmake @ONLY)

ExternalProject_Add (ffmpeg_build
        PREFIX ffmpeg_pref
        URL "${FFMPEG_SOURCE_DIR}"
        DOWNLOAD_NO_EXTRACT 1
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env PATH=${ANDROID_TOOLCHAIN_ROOT}/bin:$ENV{PATH}
                          ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/configure_ffmpeg_build.cmake"
        BUILD_COMMAND ${CMAKE_COMMAND} -E env PATH=${ANDROID_TOOLCHAIN_ROOT}/bin:$ENV{PATH}
                      make -j4
                      #${HOST_BIN}/make -j4
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env PATH=${ANDROID_TOOLCHAIN_ROOT}/bin:$ENV{PATH}
                        make install
                        #${HOST_BIN}/make install
        STEP_TARGETS ffmpeg_copy_headers
        LOG_CONFIGURE 1
        LOG_BUILD 1
        LOG_INSTALL 1
)

ExternalProject_Get_Property (ffmpeg_build SOURCE_DIR)

configure_file ("${CMAKE_CURRENT_LIST_DIR}/copy_ffmpeg_headers.cmake" copy_ffmpeg_headers.cmake @ONLY)

ExternalProject_Add_Step (
        ffmpeg_build
        ffmpeg_copy_headers
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/copy_ffmpeg_headers.cmake"
        DEPENDEES build
        DEPENDERS install
)

#

add_library (foleys_ffmpeg INTERFACE)

ExternalProject_Get_Property (ffmpeg_build INSTALL_DIR)

foreach (ffmpeg_lib IN LISTS foleys_ffmpeg_libs)

    add_library (Foleys::${ffmpeg_lib} SHARED IMPORTED)

    add_dependencies (Foleys::${ffmpeg_lib} ffmpeg_build)

    set_target_properties (Foleys::${ffmpeg_lib} PROPERTIES IMPORTED_LOCATION "${INSTALL_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${ffmpeg_lib}.${CMAKE_SHARED_LIBRARY_SUFFIX}")

    target_link_libraries (foleys_ffmpeg INTERFACE Foleys::${ffmpeg_lib})

endforeach()

#

#target_sources (foleys_ffmpeg PRIVATE ${ffmpeg_src})

add_dependencies (foleys_ffmpeg ffmpeg_build)

#target_link_libraries (foleys_ffmpeg PRIVATE ${foleys_ffmpeg_libs})

target_link_directories (foleys_ffmpeg INTERFACE "${FFMPEG_OUTPUT_DIR}" "${INSTALL_DIR}")

target_include_directories (foleys_ffmpeg INTERFACE "${FFMPEG_OUTPUT_DIR}/include")
