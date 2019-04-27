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

class AudioClip : public AVClip
{
public:
    AudioClip (VideoEngine& videoEngine);
    virtual ~AudioClip() = default;

    juce::String getDescription() const override;

    juce::File getMediaFile() const override;
    void setMediaFile (const juce::File& media);

    void setAudioFormatReader (juce::AudioFormatReader* reader);

    std::pair<int64_t, juce::Image> getFrame (double pts) const override { return {}; }
    juce::Image getCurrentFrame() const override  { return {}; }
    bool isFrameAvailable (double pts) const override { return false; }

    Size getVideoSize() const override  { return {}; }
    double getCurrentTimeInSeconds() const override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return false; }
    void setLooping (bool shouldLoop) override {}

    juce::Image getStillImage (double seconds, Size size) override { return {}; }

    double getLengthInSeconds() const override;
    Timecode getFrameTimecodeForTime (double time) const override;
    Timecode getCurrentTimecode() const override;

    bool hasVideo() const override    { return false; };
    bool hasAudio() const override    { return true; };
    bool hasSubtitle() const override { return false; };

    std::shared_ptr<AVClip> createCopy() override;

    double getSampleRate() const override { return sampleRate; }

private:

    void setupResampler();

    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::ResamplingAudioSource>   resampler;
    juce::File mediaFile;
    double sampleRate = 0.0;
    double originalSampleRate = 0.0;
    int    samplesPerBlock = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioClip)
};

}
