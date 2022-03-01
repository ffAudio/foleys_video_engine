set (CMAKE_SUPPRESS_REGENERATION TRUE)

# juce_add_plugin (PluginTest)
# target_link_libraries (PluginTest PRIVATE Foleys::foleys_video_engine)

juce_add_gui_app (AppTest)
target_link_libraries (AppTest PRIVATE Foleys::foleys_video_engine Foleys::FoleysDefaultTarget)
target_sources (AppTest PRIVATE TestAppMain.cpp)
