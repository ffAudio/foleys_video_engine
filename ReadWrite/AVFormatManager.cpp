

namespace foleys
{


std::unique_ptr<AVReader> AVFormatManager::createReaderFor (juce::File file)
{
    return std::make_unique<FFmpegReader> (file);
}

}
