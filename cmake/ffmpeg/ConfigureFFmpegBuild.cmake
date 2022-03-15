cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

if (APPLE AND NOT IOS)
	include ("${CMAKE_CURRENT_LIST_DIR}/FFmpegMacBuild.cmake")
endif()


#

# Runs FFmpeg's configure script (at CMake configure time)
function (foleys_preconfigure_ffmpeg_build)

    set (options "")
    set (oneValueArgs OUTPUT_DIR SOURCE_DIR EXTRA_ARGS C_FLAGS LD_FLAGS)
    set (multiValueArgs "")

    cmake_parse_arguments (FOLEYS_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT FOLEYS_ARG_OUTPUT_DIR)
        message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument OUTPUT_DIR!")
    endif()

    if (NOT FOLEYS_ARG_SOURCE_DIR)
        message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument SOURCE_DIR!")
    endif()

    set (CONFIGURE_COMMAND
        "./configure
        --disable-static
        --disable-doc
        --disable-asm
        --enable-shared
        --enable-protocol=file
        --shlibdir=${FOLEYS_ARG_OUTPUT_DIR}
        --libdir=${FOLEYS_ARG_OUTPUT_DIR}
        --incdir=${FOLEYS_ARG_OUTPUT_DIR}/include
        --prefix=${FOLEYS_ARG_OUTPUT_DIR}")

    if (FOLEYS_ARG_EXTRA_ARGS)
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} ${FOLEYS_ARG_EXTRA_ARGS}")
    endif()

    if (FOLEYS_ARG_C_FLAGS)
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --extra-cflags=${FOLEYS_ARG_C_FLAGS}")
    endif()

    if (FOLEYS_ARG_LD_FLAGS)
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --extra-ldflags=${FOLEYS_ARG_LD_FLAGS}")
    endif() 

    if (CMAKE_SYSROOT)
        set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --sysroot=${CMAKE_SYSROOT}")
    endif()

    if (IOS OR ANDROID)
        set (CONFIGURE_COMMAND 
                "${CONFIGURE_COMMAND}
                --enable-cross-compile
                --disable-programs
                --enable-pic"
        )

        if (IOS)
            set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --target-os=darwin")
        elseif (ANDROID)
            set (CONFIGURE_COMMAND "${CONFIGURE_COMMAND} --target-os=android")
        endif()
    endif()

    separate_arguments (ffmpeg_config_command UNIX_COMMAND "${CONFIGURE_COMMAND}")

    if (NOT FFmpeg_FIND_QUIETLY)
        set (cmd_echo_flag COMMAND_ECHO STDOUT)
    endif()

    execute_process (COMMAND ${ffmpeg_config_command}
                     WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}"
                     ${cmd_echo_flag}
                     COMMAND_ERROR_IS_FATAL ANY)

endfunction()

#

# Creates targets that execute the FFmpeg build and can be linked to from elsewhere in CMake
# BUILD_TARGET - name used for the actual underlying custom command target that executes the FFmpeg build
function (foleys_create_ffmpeg_build_target)

    set (options "")
    set (oneValueArgs BUILD_TARGET SOURCE_DIR OUTPUT_DIR)
    set (multiValueArgs "")

    cmake_parse_arguments (FOLEYS_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT FOLEYS_ARG_BUILD_TARGET)
        message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument BUILD_TARGET!")
    endif()

    if (NOT FOLEYS_ARG_SOURCE_DIR)
        message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument SOURCE_DIR!")
    endif()

    if (NOT FOLEYS_ARG_OUTPUT_DIR)
        message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument OUTPUT_DIR!")
    endif()

    #

    function (foleys_make_lib_filename libname filename_out)

        set (filename "")

        if (CMAKE_SHARED_LIBRARY_PREFIX)
            set (filename "${CMAKE_SHARED_LIBRARY_PREFIX}")
        endif()

        set (filename "${filename}${libname}")

        if (CMAKE_SHARED_LIBRARY_SUFFIX)
            set (filename "${filename}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        endif()

        set (${filename_out} "${filename}" PARENT_SCOPE)

    endfunction()

    #

    # the target should only already exist if this is a Mac universal binary build...
    if (NOT TARGET ffmpeg)
    	add_library (ffmpeg INTERFACE)
	endif()

    set (ffmpeg_libs_output_files "")

    foreach (libname ${FFMPEG_LIBS})
        foleys_make_lib_filename ("${libname}" libfilename)

        list (APPEND ffmpeg_libs_output_files "${FOLEYS_ARG_OUTPUT_DIR}/${libfilename}")

        target_link_libraries (ffmpeg INTERFACE "${FOLEYS_ARG_OUTPUT_DIR}/${libfilename}")
    endforeach()

    #

    add_custom_target ("${FOLEYS_ARG_BUILD_TARGET}"
                       COMMAND "${FFMPEG_MAKE_EXECUTABLE}" -j4
                       WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}"
                       COMMENT "Building FFmpeg..."
                       VERBATIM USES_TERMINAL)

    add_custom_command (TARGET "${FOLEYS_ARG_BUILD_TARGET}"
                        POST_BUILD
                        COMMAND "${FFMPEG_MAKE_EXECUTABLE}" install
                        BYPRODUCTS "${ffmpeg_libs_output_files}"
                        WORKING_DIRECTORY "${FOLEYS_ARG_SOURCE_DIR}"
                        VERBATIM USES_TERMINAL)

    add_dependencies (ffmpeg "${FOLEYS_ARG_BUILD_TARGET}")

endfunction()

#

function (foleys_configure_ffmpeg_build)

	set (options "")
	set (oneValueArgs SOURCE_DIR OUTPUT_DIR)
	set (multiValueArgs "")

	cmake_parse_arguments (FOLEYS_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if (NOT FOLEYS_ARG_SOURCE_DIR)
		message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument SOURCE_DIR!")
	endif()

    if (NOT FOLEYS_ARG_OUTPUT_DIR)
        set (FOLEYS_ARG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
    endif()

	if (APPLE AND NOT IOS)
		foleys_configure_ffmpeg_macos_universal_binary_build (SOURCE_DIR "${FOLEYS_ARG_SOURCE_DIR}" 
											 				  OUTPUT_DIR "${FOLEYS_ARG_OUTPUT_DIR}")
	else()
		message (STATUS "Configuring FFmpeg build...")

		foleys_preconfigure_ffmpeg_build (SOURCE_DIR "${FOLEYS_ARG_SOURCE_DIR}" 
									  	  OUTPUT_DIR "${FOLEYS_ARG_OUTPUT_DIR}")

		foleys_create_ffmpeg_build_target (SOURCE_DIR "${FOLEYS_ARG_SOURCE_DIR}" 
										   OUTPUT_DIR "${FOLEYS_ARG_OUTPUT_DIR}" 
										   BUILD_TARGET ffmpeg_build)
	endif()

	target_include_directories (ffmpeg INTERFACE "${FOLEYS_ARG_OUTPUT_DIR}/include")

endfunction()
