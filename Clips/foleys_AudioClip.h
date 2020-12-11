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

/**
 @class AudioClip

 The AudioClip plays back an audio file inside the video engine. It wraps around
 a JUCE AudioFormatReaderSource.

 When you created a shared_ptr of an AudioClip, call manageLifeTime() on the VideoEngine,
 that will add it to the auto release pool and register possible background jobs
 with the TimeSliceThreads.
 */

class AudioClip : public AVClip
{
public:
    AudioClip (VideoEngine& videoEngine);

    /** Used to identify the clip type to the user */
    juce::String getClipType() const override { return NEEDS_TRANS ("Audio"); }

    juce::String getDescription() const override;

    juce::URL getMediaFile() const override;
    void setMediaFile (const juce::URL& media);

    void setAudioFormatReader (juce::AudioFormatReader* reader, int samplesToBuffer = 48000);

    VideoFrame& getFrame ([[maybe_unused]]double pts) override { return dummy; }
    bool isFrameAvailable ([[maybe_unused]]double pts) const override { return false; }

#if FOLEYS_USE_OPENGL
    void render (double) override {}
#endif

    Size getVideoSize() const override  { return {}; }
    double getCurrentTimeInSeconds() const override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return false; }
    void setLooping ([[maybe_unused]]bool shouldLoop) override {}

    juce::Image getStillImage ([[maybe_unused]]double seconds, [[maybe_unused]]Size size) override { return {}; }

    double getLengthInSeconds() const override;

    bool hasVideo() const override    { return false; }
    bool hasAudio() const override    { return true; }

    std::shared_ptr<AVClip> createCopy (StreamTypes types) override;

    double getSampleRate() const override { return sampleRate; }

private:

    void setupResampler();

    std::unique_ptr<juce::AudioFormatReader>       reader;
    std::unique_ptr<juce::PositionableAudioSource> readerSource;
    std::unique_ptr<juce::ResamplingAudioSource>   resampler;
    VideoFrame dummy;
    juce::URL  mediaFile;
    double sampleRate = 0.0;
    double originalSampleRate = 0.0;
    int    samplesPerBlock = 0;
    float  lastGain = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioClip)
};

} // foleys
