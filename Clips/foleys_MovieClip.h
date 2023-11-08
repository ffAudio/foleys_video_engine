/*
 ==============================================================================

 Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
 All rights reserved.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 ==============================================================================
 */

#pragma once


namespace foleys
{

/**
 @class MovieClip

 The MovieClip plays back a video file. It buffers an amount of audio and
 video frames ahead of time. To check, you can call isFrameAvailable(),
 in case you need to wait for the frame, e.g. for offline bouncing.

 When you created a shared_ptr of an MovieClip, call manageLifeTime() on the VideoEngine,
 that will add it to the auto release pool and register possible background jobs
 with the TimeSliceThreads.
 */
class MovieClip   : public AVClip,
                    private juce::AsyncUpdater
{
public:
    MovieClip (VideoEngine& videoEngine);

    /** Used to identify the clip type to the user */
    juce::String getClipType() const override { return NEEDS_TRANS ("Movie"); }

    juce::String getDescription() const override;

    bool openFromFile (const juce::File file);

    juce::URL getMediaFile() const override;

    void setReader (std::unique_ptr<AVReader> reader);
    void setThumbnailReader (std::unique_ptr<AVReader> reader);

    Size getVideoSize() const override;

    double getLengthInSeconds() const override;

    double getCurrentTimeInSeconds() const override;

    VideoFrame& getFrame (double pts) override;
    bool isFrameAvailable (double pts) const override;

    void render (juce::Graphics& view, juce::Rectangle<float> area, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) override;

#if FOLEYS_USE_OPENGL
    void render (OpenGLView& view, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) override;
#endif

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping (bool shouldLoop) override;

    juce::Image getStillImage (double seconds, Size size) override;

    juce::TimeSliceClient* getBackgroundJob() override;

    bool hasVideo() const override;
    bool hasAudio() const override;

    double getFrameDurationInSeconds() const override;

    std::shared_ptr<AVClip> createCopy (StreamTypes types) override;

    int getNumChannels() const override;

    double getSampleRate() const override;

    /** When rendering non realtime (bounce), use this to wait for background
        threads to read ahead */
    bool waitForSamplesReady (int samples, int timeout=1000) override;

    bool waitForFrameReady (double pts, int timeout=1000) override;

private:

    void handleAsyncUpdate() override;

    /** @internal */
    class BackgroundReaderJob : public juce::TimeSliceClient
    {
    public:
        BackgroundReaderJob (MovieClip& owner);

        int useTimeSlice() override;

        void setSuspended (bool s);
        bool isSuspended() const;
    private:
        MovieClip& owner;
        std::atomic<bool> suspended = true;
        bool inDecodeBlock = false;
    };

    BackgroundReaderJob backgroundJob {*this};
    friend BackgroundReaderJob;

    std::unique_ptr<AVReader> movieReader;
    std::unique_ptr<AVReader> thumbnailReader;
    std::vector<juce::LagrangeInterpolator> resamplers;

    double  sampleRate = {};
    int64_t nextReadPosition = 0;
    int64_t lastShownFrame = -1;
    bool    loop = false;
    float   lastGain = 0.0;

    Size originalSize;

    VideoFifo videoFifo { 30 };
    AudioFifo audioFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MovieClip)
};


} // foleys
