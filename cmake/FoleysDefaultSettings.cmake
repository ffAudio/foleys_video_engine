cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

set_property (GLOBAL PROPERTY USE_FOLDERS YES)

set (ENV{CMAKE_EXPORT_COMPILE_COMMANDS} TRUE)
set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE CACHE INTERNAL "")

if (NOT DEFINED ENV{CMAKE_INSTALL_MODE})
	set (ENV{CMAKE_INSTALL_MODE} ABS_SYMLINK_OR_COPY)
endif()

include (CheckIPOSupported)

check_ipo_supported (RESULT result OUTPUT output)

if(result)
	set (ENV{CMAKE_INTERPROCEDURAL_OPTIMIZATION} ON)
	set (CMAKE_INTERPROCEDURAL_OPTIMIZATION ON CACHE INTERNAL "")
	message (VERBOSE "Enabling IPO")
endif()


# platform settings

if(APPLE)
	if(IOS)
		set (ENV{MACOSX_DEPLOYMENT_TARGET} 9.3)
		set (CMAKE_OSX_DEPLOYMENT_TARGET "9.3" CACHE INTERNAL "")

		set (CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO CACHE INTERNAL "")

		option (FOLEYS_IOS_SIMULATOR "Build for an iOS simulator, rather than a real device" ON)

		if(FOLEYS_IOS_SIMULATOR)
			set (CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "\"iPhone Developer\"" CACHE INTERNAL "")

			set (IOS_PLATFORM_LOCATION "iPhoneSimulator.platform" CACHE INTERNAL "")
			set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator" CACHE INTERNAL "")

			set (ENV{CMAKE_OSX_ARCHITECTURES} "i386;x86_64")
			set (CMAKE_OSX_ARCHITECTURES "i386;x86_64" CACHE INTERNAL "")

		else() # Options for building for a real device

			set (CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "" CACHE STRING "")

			set (IOS_PLATFORM_LOCATION "iPhoneOS.platform" CACHE INTERNAL "")
			set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos" CACHE INTERNAL "")

			set (ENV{CMAKE_OSX_ARCHITECTURES} "armv7;armv7s;arm64;i386;x86_64")
			set (CMAKE_OSX_ARCHITECTURES "armv7;armv7s;arm64;i386;x86_64" CACHE INTERNAL "")

		endif()
	else()
		set (ENV{MACOSX_DEPLOYMENT_TARGET} 10.11)
		set (CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "")

		set (FOLEYS_MAC_SDK_VERSION "10.13" CACHE STRING "")
		mark_as_advanced (FOLEYS_MAC_SDK_VERSION FORCE)

		set (
			MAC_SDK_DIR
			"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${FOLEYS_MAC_SDK_VERSION}.sdk"
			)

		if(IS_DIRECTORY ${MAC_SDK_DIR})
			set (CMAKE_OSX_SYSROOT ${MAC_SDK_DIR} CACHE INTERNAL "")
		else()
			message (DEBUG "Mac SDK dir ${MAC_SDK_DIR} doesn't exist!")
		endif()

		option (FOLEYS_MAC_UNIVERSAL_BINARY "Builds for x86_64 and arm64" ON)

		if(FOLEYS_MAC_UNIVERSAL_BINARY)
			set (ENV{CMAKE_OSX_ARCHITECTURES} "x86_64;arm64")
			set (CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
		endif()
	endif()
else()
	set (CMAKE_INSTALL_RPATH $ORIGIN CACHE INTERNAL "")

	if(WIN32)
		set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
	else()
		# fixes a bug with LTO on Ubuntu with Clang
		set (CMAKE_AR ${CMAKE_CXX_COMPILER_AR} CACHE PATH "AR" FORCE)
		set (CMAKE_RANLIB ${CMAKE_CXX_COMPILER_RANLIB} CACHE PATH "RANLIB" FORCE)

		mark_as_advanced (CMAKE_AR CMAKE_RANLIB FORCE)
	endif()
endif()

