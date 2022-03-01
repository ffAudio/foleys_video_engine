cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

include (FoleysDefaultSettings)

add_library (FoleysDefaultTarget INTERFACE)

set_target_properties (FoleysDefaultTarget PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_compile_features (FoleysDefaultTarget INTERFACE cxx_std_17)

if((CMAKE_CXX_COMPILER_ID MATCHES "MSVC") OR (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC"))

	# config flags
	target_compile_options (
		FoleysDefaultTarget INTERFACE $<IF:$<CONFIG:Debug>,/Od /Zi,/Ox>
									  $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:/MP> /EHsc)

	# LTO
	target_compile_options (
		FoleysDefaultTarget
		INTERFACE $<$<CONFIG:Release>:$<IF:$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">,-GL,-flto>>
		)

	target_link_libraries (
		FoleysDefaultTarget
		INTERFACE $<$<CONFIG:Release>:$<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">:-LTCG>>)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")

	# config flags
	target_compile_options (FoleysDefaultTarget INTERFACE $<$<CONFIG:Debug>:-g -O0>
														  $<$<CONFIG:Release>:-O3>)

	# LTO
	if(NOT MINGW)
		target_compile_options (FoleysDefaultTarget INTERFACE $<$<CONFIG:Release>:-flto>)
		target_link_libraries (FoleysDefaultTarget INTERFACE $<$<CONFIG:Release>:-flto>)
	endif()

endif()


# MacOS options

if(APPLE)

	set_target_properties (FoleysDefaultTarget PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
	target_compile_definitions (FoleysDefaultTarget INTERFACE JUCE_USE_VDSP_FRAMEWORK=1)

	if(IOS)

		set_target_properties (FoleysDefaultTarget PROPERTIES 
						ARCHIVE_OUTPUT_DIRECTORY "./" 
						XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
					    XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
					    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
					    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "9.3")

		target_compile_options (FoleysDefaultTarget INTERFACE "-mmacosx-version-min=9.3")
		target_link_options (FoleysDefaultTarget INTERFACE "-mmacosx-version-min=9.3")

	else()

		option (FOLEYS_MAC_UNIVERSAL_BINARY "Builds for x86_64 and arm64" ON)

		if (FOLEYS_MAC_UNIVERSAL_BINARY AND XCODE)

			execute_process (COMMAND uname -m RESULT_VARIABLE result OUTPUT_VARIABLE osx_native_arch
							 OUTPUT_STRIP_TRAILING_WHITESPACE)

			if ("${osx_native_arch}" STREQUAL "arm64")
				set_target_properties (FoleysDefaultTarget PROPERTIES OSX_ARCHITECTURES "x86_64;arm64")
				message (STATUS "Enabling Mac universal binary")
			endif()
		endif()

		set_target_properties (FoleysDefaultTarget PROPERTIES XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.11")

		target_compile_options (FoleysDefaultTarget INTERFACE "-mmacosx-version-min=10.11")
		target_link_options (FoleysDefaultTarget INTERFACE "-mmacosx-version-min=10.11")
	endif()
endif()

add_library (Foleys::FoleysDefaultTarget ALIAS FoleysDefaultTarget)
