cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

#

find_program (YASM yasm)
find_program (LIPO lipo)

if (NOT YASM OR NOT LIPO)  # install yasm and/or lipo via Homebrew
	
	if (NOT FFmpeg_FIND_QUIETLY)
		message (STATUS "Installing YASM via Homebrew...")
		set (cmd_echo_flag COMMAND_ECHO STDOUT)
	endif()

	find_program (HOMEBREW brew)

	if (NOT HOMEBREW)
		unset (CACHE{HOMEBREW})

		find_program (BASH bash)

		if (NOT BASH)
			message (FATAL_ERROR "bash is required for installing Homebrew, and cannot be found!")
		endif()

		if (NOT FFmpeg_FIND_QUIETLY)
			message (STATUS "Installing Homebrew...")
		endif()

		execute_process (COMMAND "${BASH}" "${CMAKE_CURRENT_LIST_DIR}/mac_install_homebrew.sh"
						 ${cmd_echo_flag}
						 COMMAND_ERROR_IS_FATAL ANY)

		find_program (HOMEBREW brew)

		if (NOT HOMEBREW)
			message (FATAL_ERROR "Error installing Homebrew!")
		endif()
	endif()

	execute_process (COMMAND "${HOMEBREW}" install yasm lipo
					 ${cmd_echo_flag}
					 COMMAND_ERROR_IS_FATAL ANY)
endif()

#

function (foleys_configure_ffmpeg_macos_universal_binary_build)

	set (options "")
	set (oneValueArgs SOURCE_DIR OUTPUT_DIR)
	set (multiValueArgs "")

	cmake_parse_arguments (FOLEYS_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if (NOT FOLEYS_ARG_SOURCE_DIR)
		message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument SOURCE_DIR!")
	endif()

	if (NOT FOLEYS_ARG_OUTPUT_DIR)
		message (FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} called without required argument OUTPUT_DIR!")
	endif()

	set (FFMPEG_ARCHITECTURES "x86_64;arm64")

	foreach (arch ${FFMPEG_ARCHITECTURES})

		if (NOT FFmpeg_FIND_QUIETLY)
			message (STATUS "Configuring FFmpeg build for arch ${arch}...")
		endif()

		set (sourceDir "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_arch_sources/${arch}/${FFMPEG_NAME}")

		# each arch needs its own copy of the source tree ¯\_(ツ)_/¯
		file (COPY "${FOLEYS_ARG_SOURCE_DIR}" 
			  DESTINATION "${sourceDir}/.." # silly cmake adds an extra nested directory when copying
			  USE_SOURCE_PERMISSIONS
			  FOLLOW_SYMLINK_CHAIN)

		set (outputDir "${FOLEYS_ARG_OUTPUT_DIR}/${arch}")

		foleys_preconfigure_ffmpeg_build (SOURCE_DIR "${sourceDir}"
										  OUTPUT_DIR "${outputDir}"
										  EXTRA_ARGS "--arch=${arch} --cc=clang --cxx=clang++")

		foleys_create_ffmpeg_build_target (SOURCE_DIR "${sourceDir}"
										   OUTPUT_DIR "${outputDir}"
										   BUILD_TARGET "ffmpeg_build_${arch}")

		set_target_properties (ffmpeg_build_x86_64 PROPERTIES OSX_ARCHITECTURES ${arch})
	endforeach()

	# foreach(ffmpeg_lib ${FFMPEG_LIBS})

	# 	set (libFilename "lib${ffmpeg_lib}.dylib")

	# 	set (output_file "${FOLEYS_ARG_OUTPUT_DIR}/universal/${libFilename}")

	# 	add_custom_target (ffmpeg_${ffmpeg_lib}_lipo 
	# 					   COMMAND lipo -create "${FOLEYS_ARG_OUTPUT_DIR}/x86_64/${libFilename}" "${FOLEYS_ARG_OUTPUT_DIR}/arm64/${libFilename}" -output "${output_file}"
	# 					   BYPRODUCTS "${output_file}"
	# 					   VERBATIM USES_TERMINAL
	# 					   COMMENT "FFmpeg - running lipo on ${ffmpeg_lib}...")

	# 	add_dependencies (ffmpeg_${ffmpeg_lib}_lipo ffmpeg_build_x86_64)
	# 	add_dependencies (ffmpeg_${ffmpeg_lib}_lipo ffmpeg_build_arm64)

	# 	add_dependencies (foleys_ffmpeg ffmpeg_${ffmpeg_lib}_lipo)

	# 	target_link_libraries (foleys_ffmpeg INTERFACE "${output_file}")
	# endforeach()

	# Need to manually copy headers...
	file (GLOB libs ${FOLEYS_ARG_SOURCE_DIR}/lib*)

	file (COPY ${libs} "${FOLEYS_ARG_SOURCE_DIR}/config.h" "${FOLEYS_ARG_SOURCE_DIR}/compat"
	      DESTINATION "${FOLEYS_ARG_OUTPUT_DIR}/include"
	      FILES_MATCHING PATTERN *.h)

endfunction()
