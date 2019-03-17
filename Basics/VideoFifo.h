
#pragma once

namespace foleys
{

class VideoFifo
{
public:
    VideoFifo() = default;
    ~VideoFifo() = default;

    void pushVideoFrame (juce::Image& image, juce::int64 timestamp);
    juce::Image getVideoFrame (double timestamp);

    void clearFramesOlderThan (double timestamp);

    void setTimebase (double timebase);

    void setSize (AVSize size);

private:
    double timebase = 0;
    AVSize originalSize;

    std::map<juce::int64, juce::Image> videoFrames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoFifo)
};


}
