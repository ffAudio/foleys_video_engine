
#pragma once

namespace foleys
{

class AVReader
{
public:
    AVReader() = default;
    virtual ~AVReader() = default;

    bool isOpenedOk() const;
    virtual juce::int64 getTotalLength() const = 0;

    virtual void setPosition (const juce::int64 position) = 0;

    virtual void readNewData (VideoFifo&, AudioFifo&) = 0;

    Size originalSize;
    int    pixelFormat;
    double timebase = {};

    double      sampleRate  = {};
    int         numChannels = 0;
    juce::int64 numSamples  = 0;

protected:
    bool   opened    = false;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVReader)
};

}
