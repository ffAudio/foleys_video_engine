

namespace foleys
{

void VideoFifo::pushVideoFrame (juce::Image& image, juce::int64 timestamp)
{
    videoFrames [timestamp] = image;
}

juce::Image VideoFifo::getVideoFrame (double timestamp) const
{
    auto vf = videoFrames.lower_bound (timestamp / timebase);
    if (vf != videoFrames.end())
        return vf->second;

    return {};
}

Timecode VideoFifo::getFrameTimecodeForTime (double time) const
{
    auto vf = videoFrames.lower_bound (time / timebase);
    if (vf != videoFrames.end())
        return {vf->first, timebase};

    return {};
}

void VideoFifo::clear()
{
    videoFrames.clear();
}

void VideoFifo::clearFramesOlderThan (Timecode timecode)
{
    auto current = videoFrames.find (timecode.count);
    if (current == videoFrames.begin())
        return;

    videoFrames.erase (videoFrames.begin(), --current);
}

void VideoFifo::setTimebase (double timebaseToUse)
{
    timebase = timebaseToUse;
}

void VideoFifo::setSize (Size size)
{
    originalSize = size;
}

}
