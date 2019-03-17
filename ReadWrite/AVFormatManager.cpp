

namespace foleys
{


std::unique_ptr<AVReader> AVFormatManager::createReaderFor (juce::File file)
{
#if FOLEYS_USE_FFMPEG
    return std::make_unique<FFmpegReader> (file);
#endif

    return {};
}

}
