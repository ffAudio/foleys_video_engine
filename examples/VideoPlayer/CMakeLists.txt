cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(FoleysVideoPlayer VERSION 1.0.0 LANGUAGES CXX OBJC C)

juce_add_gui_app(${PROJECT_NAME}
        PRODUCT_NAME ${PROJECT_NAME}
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

find_package (ffmpeg)

add_subdirectory(Source)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Foleys::foleys_video_engine
        Foleys::ff_meters
        ffmpeg::ffmpeg
        juce_audio_utils
        juce_opengl
        juce_gui_extra
        juce_cryptography
        juce_audio_processors
        juce_audio_devices
        juce_audio_formats)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        FOLEYS_USE_OPENGL=0
        FOLEYS_CAMERA_SUPPORT=0
        USE_FF_AUDIO_METERS=1)

juce_generate_juce_header(${PROJECT_NAME})
