/*
 ==============================================================================

 Copyright (c) 2019, Foleys Finest Audio - Daniel Walz
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

class AVMovieClip : public AVClip,
                    private juce::AsyncUpdater
{
public:
    AVMovieClip (VideoEngine& videoEngine);
    virtual ~AVMovieClip() = default;

    juce::String getDescription() const override;

    bool openFromFile (const juce::File file);

    void setReader (std::unique_ptr<AVReader> reader);
    void setThumbnailReader (std::unique_ptr<AVReader> reader);

    Size getVideoSize() const override;

    double getLengthInSeconds() const override;

    Timecode getCurrentTimecode() const override;
    double getCurrentTimeInSeconds() const override;

    Timecode getFrameTimecodeForTime (double time) const override;

    juce::Image getFrame (double pts) const override;

    juce::Image getCurrentFrame() const override;

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
    bool hasSubtitle() const override;

    double getSampleRate() const override;

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
        bool inDecodeBlock = false;
    };

    BackgroundReaderJob backgroundJob {*this};
    friend BackgroundReaderJob;

    std::unique_ptr<AVReader> movieReader;
    std::unique_ptr<AVReader> thumbnailReader;
    std::vector<juce::LagrangeInterpolator> resamplers;

    double      sampleRate = {};
    juce::int64 nextReadPosition = 0;
    Timecode    lastShownFrame;
    bool        loop = false;

    Size originalSize;

    Timecode originalLength;

    VideoFifo videoFifo;
    AudioFifo audioFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVMovieClip)
};


} // foleys
