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
 @class ImageClip

 This class delivers a still image as video.

 When you created a shared_ptr of an ImageClip, call manageLifeTime() on the VideoEngine,
 that will add it to the auto release pool and register possible background jobs
 with the TimeSliceThreads.
 */
class ImageClip : public AVClip
{
public:
    ImageClip (VideoEngine& videoEngine);

    /** Used to identify the clip type to the user */
    juce::String getClipType() const override { return NEEDS_TRANS ("Still Image"); }

    juce::String getDescription() const override;

    juce::URL getMediaFile() const override;
    void setMediaFile (const juce::URL& media);

    void setImage (const juce::Image& image);

    VideoFrame& getFrame (double pts) override;
    bool isFrameAvailable ([[maybe_unused]]double pts) const override { return frame.image.isValid(); }

    void render (juce::Graphics& view, juce::Rectangle<float> area, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) override;

#if FOLEYS_USE_OPENGL
    void render (OpenGLView& view, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) override;
#endif

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

    bool hasVideo() const override    { return true; }
    bool hasAudio() const override    { return false; }

    std::shared_ptr<AVClip> createCopy (StreamTypes types) override;

    double getSampleRate() const override;

private:

    juce::URL           mediaFile;
    VideoFrame          frame;
    VideoStreamSettings videoSettings;

    double sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageClip)
};

} // foleys
