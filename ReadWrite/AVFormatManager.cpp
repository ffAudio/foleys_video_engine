

namespace foleys
{


std::unique_ptr<AVReader> AVFormatManager::createReaderFor (juce::File file, StreamTypes type)
{
#if FOLEYS_USE_FFMPEG
    return std::make_unique<FFmpegReader> (file, type);
#endif

    return {};
}

}
