cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (CPackComponent)
include (GNUInstallDirs)
include (ProcessorCount)

ProcessorCount (NUM_PROCESSORS)

if(NOT (NUM_PROCESSORS GREATER 0))
    set (NUM_PROCESSORS 4)
endif()

#

#[[
    preconfigure_ffmpeg_build (OUTPUT_DIR <dir>
                               SOURCE_DIR <dir>
                              [EXTRA_ARGS <args...>])

    This function runs ffmpeg's configure script at CMake configure time.

    By default, shared libraries are enabled and static libraries are disabled.

    EXTRA_ARGS will be appended to the configure script command.

    SOURCE_DIR must be the root of the ffmpeg source code. 
    Because their configure script works in-place, if multiple configurations
    of ffmpeg are needed in a single build (ie, for a mac universal binary),
    then multiple copies of the ffmpeg source tree must be created, and each 
    call to this function must be given a unique SOURCE_DIR to use.

    OUTPUT_DIR is an intermediate staging directory. ffmpeg is built by running
    make, and then make install is run to install its artefacts to the OUTPUT_DIR.
    OUTPUT_DIR should still be within the project's build tree.
]]
function (preconfigure_ffmpeg_build)

    set (oneValueArgs OUTPUT_DIR SOURCE_DIR)
    set (multiValueArgs EXTRA_ARGS)

    cmake_parse_arguments (FOLEYS_ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(req_var IN ITEMS SOURCE_DIR OUTPUT_DIR)
        if(NOT FOLEYS_ARG_${req_var})
            message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument ${req_var}!")
        endif()
    endforeach()

    # clean first
    execute_process (COMMAND "${FFMPEG_MAKE_EXECUTABLE}" distclean
                     WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}" 
                     COMMAND_ECHO STDOUT)

    file (REMOVE_RECURSE "${FOLEYS_ARG_OUTPUT_DIR}")

    set (CONFIGURE_COMMAND
         "./configure
         --disable-static
         --disable-doc
         --disable-asm
         --enable-shared
         --shlibdir=${FOLEYS_ARG_OUTPUT_DIR}
         --libdir=${FOLEYS_ARG_OUTPUT_DIR}
         --incdir=${FOLEYS_ARG_OUTPUT_DIR}/${CMAKE_INSTALL_INCLUDEDIR}
         --prefix=${FOLEYS_ARG_OUTPUT_DIR}")

    if (IOS OR ANDROID)
        set (
            CONFIGURE_COMMAND
            "${CONFIGURE_COMMAND}
                --enable-cross-compile
                --disable-programs
                --enable-pic")

        if (IOS)
            set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --target-os=darwin")
        else ()
            set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --target-os=android")
        endif ()
    endif ()

    separate_arguments (ffmpeg_config_command UNIX_COMMAND "${CONFIGURE_COMMAND}")

    if (FOLEYS_ARG_EXTRA_ARGS)
        list (APPEND ffmpeg_config_command ${FOLEYS_ARG_EXTRA_ARGS})
    endif ()

    execute_process (
        COMMAND ${ffmpeg_config_command} 
        WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}" 
        COMMAND_ECHO STDOUT 
        COMMAND_ERROR_IS_FATAL ANY)

endfunction ()

#

#[[
    create_ffmpeg_build_target (BUILD_TARGET <targetName> [ALL]
                                SOURCE_DIR <dir>
                                OUTPUT_DIR <dir>
                               [ARCH <archName>])

    This function creates a custom target to execute the ffmpeg build,
    and sets up some install rules.

    preconfigure_ffmpeg_build() must be run first.

    The same SOURCE_DIR and OUTPUT_DIR that were passed to preconfigure_ffmpeg_build()
    should be passed to this function.

    BUILD_TARGET is the name of the custom target that will actually execute the build.

    The ALL flag can be given to include the BUILD_TARGET in the default build.

    To facilitate building Mac universal binaries, ARCH can be given to indicate what 
    architecture this target will build, but this argument is optional.
]]
function (create_ffmpeg_build_target)

    set (options ALL)
    set (oneValueArgs BUILD_TARGET SOURCE_DIR OUTPUT_DIR ARCH)

    cmake_parse_arguments (FOLEYS_ARG "${options}" "${oneValueArgs}" "" ${ARGN})

    foreach(req_var IN ITEMS BUILD_TARGET SOURCE_DIR OUTPUT_DIR)
        if(NOT FOLEYS_ARG_${req_var})
            message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument ${req_var}!")
        endif()
    endforeach()

    #

    function (__ffmpeg_make_lib_filename libname filename_out)

        set (filename "${libname}")

        if (CMAKE_SHARED_LIBRARY_PREFIX)
            set (filename "${CMAKE_SHARED_LIBRARY_PREFIX}${filename}")
        endif ()

        if (CMAKE_SHARED_LIBRARY_SUFFIX)
            set (filename "${filename}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        endif ()

        set (${filename_out} "${filename}" PARENT_SCOPE)

    endfunction ()

    #

    set (ffmpeg_libs_output_files "")

    foreach (libname IN ITEMS avutil swresample avcodec avformat swscale)

        __ffmpeg_make_lib_filename ("${libname}" libfilename)

        set (lib_path "${FOLEYS_ARG_OUTPUT_DIR}/${libfilename}")

        message (TRACE "${libname}: output path is ${lib_path}")

        list (APPEND ffmpeg_libs_output_files "${lib_path}")

        if (FOLEYS_ARG_ARCH)
            set (install_dest "${CMAKE_INSTALL_LIBDIR}/${FOLEYS_ARG_ARCH}")
        else ()
            set (install_dest "${CMAKE_INSTALL_LIBDIR}")
        endif ()

        message (TRACE "${libname}: install destination is ${install_dest}")

        target_link_libraries (
            ffmpeg INTERFACE "$<BUILD_INTERFACE:${lib_path}>"
                             "$<INSTALL_INTERFACE:${install_dest}/${libfilename}>")

        install (FILES "${lib_path}" 
                 DESTINATION "${install_dest}" 
                 COMPONENT ffmpeg_${libname})

        cpack_add_component (ffmpeg_${libname} 
                             DISPLAY_NAME "FFmpeg ${libname} library" 
                             GROUP ffmpeg
                             DEPENDS ffmpeg_base)
    endforeach ()

    #

    if (FOLEYS_ARG_ARCH)
        set (build_comment "Building FFmpeg for arch ${FOLEYS_ARG_ARCH}...")
        set (install_comment "Copying built ffmpeg files for arch ${FOLEYS_ARG_ARCH}...")
    else ()
        set (build_comment "Building FFmpeg...")
        set (install_comment "Copying built ffmpeg files...")
    endif ()

    if (FOLEYS_ARG_ALL)
        set (all_flag ALL)
    else()
        unset (all_flag)
    endif ()

    add_custom_target (
        "${FOLEYS_ARG_BUILD_TARGET}"
        ${all_flag}
        COMMAND "${FFMPEG_MAKE_EXECUTABLE}" "-j${NUM_PROCESSORS}"
        WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}"
        BYPRODUCTS ${ffmpeg_libs_output_files}
        COMMENT "${build_comment}"
        VERBATIM 
        USES_TERMINAL)

    add_custom_command (
        TARGET "${FOLEYS_ARG_BUILD_TARGET}"
        POST_BUILD
        COMMAND "${FFMPEG_MAKE_EXECUTABLE}" install
        WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}"
        COMMENT "${install_comment}"
        VERBATIM 
        USES_TERMINAL)

    add_dependencies (ffmpeg "${FOLEYS_ARG_BUILD_TARGET}")

endfunction ()
