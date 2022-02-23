cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

find_package (PkgConfig)

if (NOT PkgConfig_FOUND)
	return()
endif()

list (TRANSFORM foleys_ffmpeg_libs 
	  PREPEND lib 
	  OUTPUT_VARIABLE libav_libs)

pkg_check_modules (LIBAV IMPORTED_TARGET ${libav_libs})

if (NOT LIBAV_FOUND)
	return()
endif()

add_library (foleys_ffmpeg INTERFACE)

target_link_libraries (foleys_ffmpeg INTERFACE PkgConfig::LIBAV)
