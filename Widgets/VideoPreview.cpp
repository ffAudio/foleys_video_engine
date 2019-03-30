
namespace foleys
{

VideoPreview::VideoPreview()
{
}

VideoPreview::~VideoPreview()
{
    if (clip)
    {
        clip->removeTimecodeListener (this);
        clip->removeSubtitleListener (this);
    }
}

void VideoPreview::setClip (AVClip* clipToUse)
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

AVClip* VideoPreview::getClip() const
{
    return clip;
}

void VideoPreview::paint (juce::Graphics& g)
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

void VideoPreview::timecodeChanged (Timecode tc)
{
    if (tc.count > subtitleClear.count)
    {
        subtitle.clear();
        subtitleClear.count = 0;
        currentFrameCount = -1;
    }

    if (currentFrameCount != tc.count)
    {
        currentFrameCount = tc.count;
        repaint();
    }
}

void VideoPreview::setSubtitle (const juce::String& text, Timecode until)
{
    subtitle = text;
    subtitleClear = until;
}

}
