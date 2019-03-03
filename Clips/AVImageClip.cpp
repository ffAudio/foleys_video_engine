
namespace foleys
{

void AVImageClip::setImage (const juce::Image& imageToUse)
{
    image = imageToUse;
}

juce::Image AVImageClip::getFrame (const AVTimecode) const
{
    return image;
}

juce::Image AVImageClip::getCurrentFrame() const
{
    return image;
}

AVSize AVImageClip::getOriginalSize() const
{
    return { image.getWidth(), image.getHeight() };
}

double AVImageClip::getCurrentTimeInSeconds() const
{
    return 0;
}

void AVImageClip::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{

}

void AVImageClip::releaseResources()
{

}

void AVImageClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
}

void AVImageClip::setNextReadPosition (juce::int64)
{
}

juce::int64 AVImageClip::getNextReadPosition() const
{
    return 0;
}

juce::int64 AVImageClip::getTotalLength() const
{
    return std::numeric_limits<juce::int64>::max();
}

bool AVImageClip::isLooping() const
{
    return true;
}

void AVImageClip::setLooping (bool)
{
}


} // end namespace foleys
