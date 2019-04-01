
#pragma once


namespace foleys
{

    class VideoEngine  : public juce::DeletedAtShutdown
{
public:
    VideoEngine();
    ~VideoEngine();

    void addAVClip (AVClip& clip);
    void removeAVClip (AVClip& clip);

    JUCE_DECLARE_SINGLETON (VideoEngine, true)
private:
    std::vector<std::unique_ptr<juce::TimeSliceThread>> readingThreads;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoEngine)
};

}
