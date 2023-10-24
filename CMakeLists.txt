cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    project (FoleysVideoEngine 
            VERSION 0.2.0 
            LANGUAGES CXX OBJC C
            DESCRIPTION "A video engine module for JUCE"
            HOMEPAGE_URL "https://foleysfinest.com/foleys_video_engine/")
else()
    project (FoleysVideoEngine 
            VERSION 0.2.0 
            LANGUAGES CXX C
            DESCRIPTION "A video engine module for JUCE"
            HOMEPAGE_URL "https://foleysfinest.com/foleys_video_engine/")
endif()

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT ffmpeg_FOUND)
    find_package (ffmpeg REQUIRED)
endif()

if(PROJECT_IS_TOP_LEVEL)
    message(STATUS "No parent project found, requiring JUCE")
    find_package (JUCE 6 REQUIRED)
endif()

juce_add_module ("${CMAKE_CURRENT_LIST_DIR}" ALIAS_NAMESPACE Foleys)

target_compile_definitions (foleys_video_engine INTERFACE 
                JUCE_MODAL_LOOPS_PERMITTED=1 
                JUCE_STRICT_REFCOUNTEDPOINTER=1 
                JUCE_PLUGINHOST_AU=1 
                JUCE_PLUGINHOST_VST3=1 
                JUCE_PLUGINHOST_LADSPA=1)

target_link_libraries (foleys_video_engine INTERFACE FFmpeg::FFmpeg)

#

set (FVE_INSTALL_DEST "${CMAKE_INSTALL_LIBDIR}/cmake/foleys_video_engine"
        CACHE STRING
        "Directory below INSTALL_PREFIX where the foleys_video_engine CMake package files will be installed to")

install (DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
         DESTINATION "${FVE_INSTALL_DEST}/.."
         COMPONENT foleys_video_engine
         PATTERN *.md EXCLUDE
         PATTERN .git/* EXCLUDE
         PATTERN .github/* EXCLUDE
         PATTERN *.json EXCLUDE
         PATTERN CMakeLists.txt EXCLUDE 
         PATTERN "${CMAKE_CURRENT_BINARY_DIR}/" EXCLUDE)

include (CPackComponent)

cpack_add_component (foleys_video_engine
            GROUP Foleys 
            INSTALL_TYPES Developer)

include (CMakePackageConfigHelpers)

write_basic_package_version_file (foleys_video_engine-config-version.cmake 
                                  VERSION "${PROJECT_VERSION}"
                                  COMPATIBILITY SameMajorVersion
                                  ARCH_INDEPENDENT)

configure_package_config_file (cmake/config.cmake foleys_video_engine-config.cmake
                               INSTALL_DESTINATION "${FVE_INSTALL_DEST}" 
                               NO_SET_AND_CHECK_MACRO)

install (FILES "${CMAKE_CURRENT_BINARY_DIR}/foleys_video_engine-config-version.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/foleys_video_engine-config.cmake" 
        DESTINATION "${FVE_INSTALL_DEST}"
        COMPONENT foleys_video_engine)

export (PACKAGE foleys_video_engine)
