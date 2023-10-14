cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(FoleysVideoEditor VERSION 0.2.0 LANGUAGES CXX OBJC C)

juce_add_gui_app(${PROJECT_NAME}
        PRODUCT_NAME ${PROJECT_NAME}
        VERSION "${PROJECT_VERSION}"
        MICROPHONE_PERMISSION_ENABLED ON
        CAMERA_PERMISSION_ENABLED ON
        SEND_APPLE_EVENTS_PERMISSION_ENABLED OFF
        DOCUMENT_BROWSER_ENABLED ON
        ICON_BIG "${CMAKE_CURRENT_LIST_DIR}/../Resources/FF-Logo.png"
        BUNDLE_ID "com.foleysfinest.FoleysVideoEditor"
        COMPANY_COPYRIGHT "2019-2023 by Foleys Finest Audio"
        COMPANY_NAME "Foleys Finest Audio"
        COMPANY_WEBSITE "https://foleysfinest.com"
        COMPANY_EMAIL "info@foleysfinest.com"
        PLUGINHOST_AU TRUE)

add_subdirectory(Source)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Foleys::foleys_video_engine
        Foleys::ff_meters
        juce_audio_utils
        juce_opengl
        juce_gui_extra
        juce_cryptography
        juce_audio_processors
        juce_audio_devices
        juce_audio_formats)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_LADSPA=1
        FOLEYS_USE_OPENGL=0
        FOLEYS_CAMERA_SUPPORT=0
        USE_FF_AUDIO_METERS=1)

juce_generate_juce_header(${PROJECT_NAME})
