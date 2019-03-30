
#pragma once

namespace foleys
{

class VideoPreview  : public juce::Component,
                      public AVClip::TimecodeListener,
                      public AVClip::SubtitleListener
{
public:
    VideoPreview();

    virtual ~VideoPreview();

    void setClip (AVClip* clip);

    AVClip* getClip() const;

    void paint (juce::Graphics& g) override;

    void timecodeChanged (Timecode tc) override;

    void setSubtitle (const juce::String& text, Timecode until) override;

private:
    juce::WeakReference<AVClip> clip;
    juce::RectanglePlacement placement { juce::RectanglePlacement::centred };

    juce::int64  currentFrameCount = -1;
    juce::String subtitle;
    Timecode subtitleClear;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoPreview)
};

}
