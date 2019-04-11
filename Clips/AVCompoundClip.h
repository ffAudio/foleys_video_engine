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

class AVCompoundClip  : public AVClip,
                        private juce::AsyncUpdater
{
public:
    AVCompoundClip();
    virtual ~AVCompoundClip() = default;

    juce::String getDescription() const override;

    juce::Image getFrame (double pts) const override;
    juce::Image getCurrentFrame() const override;

    Size getVideoSize() const override;
    double getCurrentTimeInSeconds() const override;

    juce::Image getStillImage (double seconds, Size size) override;

    double getLengthInSeconds() const override;
    Timecode getFrameTimecodeForTime (double time) const override;
    Timecode getCurrentTimecode() const override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;
    void setNextReadPosition (juce::int64 samples) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override;
    void setLooping (bool shouldLoop) override;

    bool hasVideo() const override;
    bool hasAudio() const override;
    bool hasSubtitle() const override;

    juce::TimeSliceClient* getBackgroundJob() override;

    struct ClipDescriptor
    {
        ClipDescriptor (std::shared_ptr<AVClip> clip);

        juce::String name;

        /** start of the clip in samples */
        std::atomic<juce::int64> start {0};

        /** length of the clip in samples */
        std::atomic<juce::int64> length {0};

        /** offset in samples */
        std::atomic<juce::int64> offset {0};

        std::shared_ptr<AVClip> clip;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipDescriptor)
    };

    std::shared_ptr<ClipDescriptor> addClip (std::shared_ptr<AVClip> clip, double start, double length = -1, double offset = 0);

    std::vector<std::shared_ptr<ClipDescriptor>> getClips() const;

private:

    void handleAsyncUpdate() override;

    std::vector<std::shared_ptr<ClipDescriptor>> getActiveClips (std::function<bool(AVCompoundClip::ClipDescriptor&)> selector) const;

    class ComposingThread : public juce::TimeSliceClient
    {
    public:
        ComposingThread (AVCompoundClip& owner);
        int useTimeSlice() override;

        void setSuspended (bool s);

    private:
        AVCompoundClip& owner;
        bool suspended = true;
        bool inRenderBlock = false;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComposingThread)
    };

    juce::CriticalSection clipDescriptorLock;

    VideoFifo videoFifo;
    ComposingThread videoRenderJob;

    std::unique_ptr<CompositingContext> composer;
    std::vector<std::shared_ptr<ClipDescriptor>> clips;
    std::atomic<juce::int64> position = {};
    Size videoSize;
    double sampleRate = 0;
    juce::AudioBuffer<float> buffer;
    Timecode lastShownFrame;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVCompoundClip)
};

} // foleys
