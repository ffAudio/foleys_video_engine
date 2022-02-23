cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

include_guard (GLOBAL)

if (NOT DEFINED ENV{CPM_SOURCE_CACHE})
	set (ENV{CPM_SOURCE_CACHE} "${PROJECT_SOURCE_DIR}/Cache")
endif()

if (NOT COMMAND CPMAddPackage)
	include ("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")
endif()
