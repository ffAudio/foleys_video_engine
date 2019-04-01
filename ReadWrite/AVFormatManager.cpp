

namespace foleys
{


std::unique_ptr<AVClip> AVFormatManager::createClipFromFile (juce::File file)
{
    auto image = juce::ImageFileFormat::loadFrom (file);
    if (image.isValid())
    {
        auto clip = std::make_unique<AVImageClip>();
        clip->setImage (image);
        return clip;
    }

    auto reader = AVFormatManager::createReaderFor (file);
    if (reader->isOpenedOk())
    {
        auto clip = std::make_unique<AVMovieClip>();
        clip->setReader (std::move (reader));
        return clip;
    }

    return {};
}


std::unique_ptr<AVReader> AVFormatManager::createReaderFor (juce::File file, StreamTypes type)
{

#if FOLEYS_USE_FFMPEG
    return std::make_unique<FFmpegReader> (file, type);
#endif

    return {};
}

}
