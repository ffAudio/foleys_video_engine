
namespace foleys
{

AVComponent::~AVComponent()
{
    if (clip)
    {
        clip->removeTimecodeListener (this);
        clip->removeSubtitleListener (this);
    }
}

void AVComponent::setClip (AVClip* clipToUse)
{
    if (clip)
    {
        clip->removeTimecodeListener (this);
        clip->removeSubtitleListener (this);
    }

    clip = clipToUse;

    if (clip)
    {
        clip->addTimecodeListener (this);
        clip->addSubtitleListener (this);
    }
}

AVClip* AVComponent::getClip() const
{
    return clip;
}

void AVComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (clip)
    {
        auto image = clip->getCurrentFrame();
        g.drawImage (image, getLocalBounds().toFloat(), placement);
    }

    if (subtitle.isNotEmpty())
    {
        g.drawFittedText (subtitle,
                          getLocalBounds().withTop (getHeight() * 0.9),
                          juce::Justification::centred, 3);
    }
}

void AVComponent::timecodeChanged (AVTimecode tc)
{
    if (tc.count > subtitleClear.count)
    {
        subtitle.clear();
        subtitleClear.count = 0;
    }
    repaint();
}

void AVComponent::setSubtitle (const juce::String& text, AVTimecode until)
{
    subtitle = text;
    subtitleClear = until;
}

}
