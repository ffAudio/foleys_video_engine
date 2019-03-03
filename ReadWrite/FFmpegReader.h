
#pragma once


#if FOLEYS_USE_FFMPEG
namespace foleys
{

class FFmpegReader : public AVReader
{
public:
    FFmpegReader (juce::File file);
    ~FFmpegReader();

private:
    class Pimpl;
    friend Pimpl;

    Pimpl* pimpl = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegReader)
};

}
#endif
