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
 class AVImageClip

 This class delivers a still image as video.
 */
class AVImageClip : public AVClip
{
public:
    AVImageClip() = default;
    virtual ~AVImageClip() = default;

    juce::String getDescription() const override;

    void setMediaFile (const juce::File& media);

    void setImage (const juce::Image& image);

    juce::Image getFrame (double pts) const override;
    juce::Image getCurrentFrame() const override;

    Size getVideoSize() const override;
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
    juce::File  mediaFile;

    double sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVImageClip)
};

} // foleys
