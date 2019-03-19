
#pragma once


namespace foleys
{

class AVMovieClip : public AVClip,
                    private juce::AsyncUpdater
{
public:
    AVMovieClip() = default;
    virtual ~AVMovieClip() = default;

    bool openFromFile (const juce::File file);

    AVSize getOriginalSize() const override;

    double getLengthInSeconds() const override;

    Timecode getCurrentTimecode() const override;
    double getCurrentTimeInSeconds() const override;

    Timecode getFrameTimecodeForTime (double time) const override;


    juce::Image getFrame (const Timecode) const override;

    juce::Image getCurrentFrame() const override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping (bool shouldLoop) override;

    juce::TimeSliceClient* getBackgroundJob() override;

private:

    void handleAsyncUpdate() override;

    class BackgroundReaderJob : public juce::TimeSliceClient
    {
    public:
        BackgroundReaderJob (AVMovieClip& owner);
        virtual ~BackgroundReaderJob() = default;

        int useTimeSlice() override;

        void setSuspended (bool s);
    private:
        AVMovieClip& owner;
        bool suspended = true;
    };

    BackgroundReaderJob backgroundJob {*this};
    friend BackgroundReaderJob;

    std::unique_ptr<AVReader> movieReader;
    std::vector<juce::LagrangeInterpolator> resamplers;

    double      sampleRate = {};
    juce::int64 nextReadPosition = 0;
    Timecode    lastShownFrame;
    bool        loop = false;

    AVSize originalSize;

    Timecode originalLength;

    VideoFifo videoFifo;
    AudioFifo audioFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVMovieClip)
};


}
