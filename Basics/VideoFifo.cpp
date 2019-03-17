

namespace foleys
{

void VideoFifo::pushVideoFrame (juce::Image& image, juce::int64 timestamp)
{
    videoFrames [timestamp] = image;
}

juce::Image VideoFifo::getVideoFrame (double timestamp)
{
    auto vf = videoFrames.lower_bound (timestamp * timebase);
    if (vf != videoFrames.end())
        return vf->second;

    return {};
}

void VideoFifo::clearFramesOlderThan (double timestamp)
{
    juce::int64 ts = timestamp * timebase;
    videoFrames.erase (videoFrames.begin(), videoFrames.lower_bound (ts));
}

void VideoFifo::setTimebase (double timebaseToUse)
{
    timebase = timebaseToUse;
}

void VideoFifo::setSize (AVSize size)
{
    originalSize = size;
}

}
