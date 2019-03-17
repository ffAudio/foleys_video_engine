
#pragma once


#if FOLEYS_USE_FFMPEG
namespace foleys
{

class FFmpegReader : public AVReader
{
public:
    FFmpegReader (juce::File file);
    ~FFmpegReader();

    juce::int64 getTotalLength() const override;

    void setPosition (const juce::int64 position) override;

    void readNewData (VideoFifo&, AudioFifo&) override;

private:
    class Pimpl;
    friend Pimpl;

    Pimpl* pimpl = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegReader)
};

}
#endif
