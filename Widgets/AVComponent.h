
#pragma once

namespace foleys
{

class AVComponent  : public juce::Component,
                     public AVClip::TimecodeListener,
                     public AVClip::SubtitleListener
{
public:
    AVComponent() = default;

    virtual ~AVComponent();

    void setClip (AVClip* clip);

    AVClip* getClip() const;

    void paint (juce::Graphics& g) override;

    void timecodeChanged (AVTimecode) override;

    void setSubtitle (const juce::String& text, AVTimecode until) override;


private:
    juce::WeakReference<AVClip> clip;
    juce::RectanglePlacement placement { juce::RectanglePlacement::centred };

    juce::String subtitle;
    AVTimecode subtitleClear;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVComponent)
};

}
