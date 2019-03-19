
#pragma once

namespace foleys
{

class VideoFifo
{
public:
    VideoFifo() = default;
    ~VideoFifo() = default;

    void pushVideoFrame (juce::Image& image, juce::int64 timestamp);
    juce::Image getVideoFrame (double timestamp) const;

    Timecode getFrameTimecodeForTime (double time) const;

    void clearFramesOlderThan (Timecode timecode);

    void setTimebase (double timebase);

    void setSize (Size size);

private:
    double timebase = 0;
    Size originalSize;

    std::map<juce::int64, juce::Image> videoFrames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoFifo)
};


}
