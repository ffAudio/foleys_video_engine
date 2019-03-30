
#pragma once


#if FOLEYS_USE_FFMPEG
namespace foleys
{

class FFmpegReader : public AVReader
{
public:
    FFmpegReader (const juce::File& file, StreamTypes type);
    ~FFmpegReader();

    juce::int64 getTotalLength() const override;

    void setPosition (const juce::int64 position) override;

    juce::Image getStillImage (double seconds, Size size) override;

    void readNewData (VideoFifo&, AudioFifo&) override;

    bool hasVideo() const override;
    bool hasAudio() const override;
    bool hasSubtitle() const override;

private:
    class Pimpl;
    friend Pimpl;

    Pimpl* pimpl = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegReader)
};

}
#endif