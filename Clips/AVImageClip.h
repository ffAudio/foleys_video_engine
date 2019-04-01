
#pragma once

namespace foleys
{

/**
 class AVImageClip

 This class delivers a still image as video.
 */
class AVImageClip : public AVClip
{
public:
    AVImageClip() = default;
    virtual ~AVImageClip() = default;

    void setImage (const juce::Image& image);

    juce::Image getFrame (const Timecode) const override;
    juce::Image getCurrentFrame() const override;

    Size getOriginalSize() const override;
    double getCurrentTimeInSeconds() const override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping (bool shouldLoop) override;

    juce::Image getStillImage (double seconds, Size size) override;

    double getLengthInSeconds() const override;
    Timecode getFrameTimecodeForTime (double time) const override;
    Timecode getCurrentTimecode() const override;


    bool hasVideo() const override    { return true; };
    bool hasAudio() const override    { return false; };
    bool hasSubtitle() const override { return false; };

private:

    juce::Image image;

    double sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVImageClip)
};

}
