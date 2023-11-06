cmake_minimum_required(VERSION 3.12)

project(MULTI_CAM_TEST VERSION 0.0.1)
set(CMAKE_BINARY_DIR build)

set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules" CACHE INTERNAL
        "The path to JUCE modules")

juce_add_gui_app(${PROJECT_NAME}
        PRODUCT_NAME "Multi Camera Test"
        VERSION "${PROJECT_VERSION}"
        MICROPHONE_PERMISSION_ENABLED ON
        CAMERA_PERMISSION_ENABLED ON
        SEND_APPLE_EVENTS_PERMISSION_ENABLED OFF
        DOCUMENT_BROWSER_ENABLED ON
        BUNDLE_ID "com.foleysfinest.FoleysVideoPlayer"
        ICON_BIG "${CMAKE_CURRENT_LIST_DIR}/../Resources/FF-Logo.png"
        COMPANY_COPYRIGHT "2019-2023 by Foleys Finest Audio"
        COMPANY_NAME "Foleys Finest Audio"
        COMPANY_WEBSITE "https://foleysfinest.com"
        COMPANY_EMAIL "info@foleysfinest.com")

target_sources(${PROJECT_NAME}
        PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        Foleys::foleys_video_engine
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        FOLEYS_USE_OPENGL=0
        FOLEYS_USE_FFMPEG=0
        FOLEYS_CAMERA_SUPPORT=1
        USE_FF_AUDIO_METERS=0)

juce_generate_juce_header(${PROJECT_NAME})
