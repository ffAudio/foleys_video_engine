
namespace foleys
{

bool AVMovieClip::openFromFile (const juce::File file)
{
    return false;
}

AVSize AVMovieClip::getOriginalSize() const
{
    return originalSize;
}

double AVMovieClip::getLengthInSeconds() const
{
    return 0;
}

double AVMovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

AVTimecode AVMovieClip::getCurrentTimecode() const
{
    return {}; // FIXME
}

juce::Image AVMovieClip::getFrame (const AVTimecode) const
{
    return {};
}

juce::Image AVMovieClip::getCurrentFrame() const
{
    return {};
}

void AVMovieClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
}

void AVMovieClip::releaseResources()
{
}

void AVMovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    nextReadPosition += info.numSamples;
}

void AVMovieClip::setNextReadPosition (juce::int64 samples)
{
    nextReadPosition = 0;
}

juce::int64 AVMovieClip::getNextReadPosition() const
{
    return nextReadPosition;
}

juce::int64 AVMovieClip::getTotalLength() const
{
    return 0;
}

bool AVMovieClip::isLooping() const
{
    return loop;
}

void AVMovieClip::setLooping (bool shouldLoop)
{
    loop = shouldLoop;
}

}
