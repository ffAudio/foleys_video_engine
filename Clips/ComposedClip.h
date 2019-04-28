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

class ComposedClip  : public AVClip,
                      private juce::AsyncUpdater,
                      private juce::ValueTree::Listener
{
public:
    ComposedClip (VideoEngine& videoEngine);
    virtual ~ComposedClip() = default;

    juce::String getDescription() const override;

    std::pair<int64_t, juce::Image> getFrame (double pts) const override;
    bool isFrameAvailable (double pts) const override;

    juce::Image getCurrentFrame() const override;

    Size getVideoSize() const override;
    double getCurrentTimeInSeconds() const override;

    juce::Image getStillImage (double seconds, Size size) override;

    double getLengthInSeconds() const override;

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

    std::shared_ptr<AVClip> createCopy() override;

    double getSampleRate() const override;

    juce::TimeSliceClient* getBackgroundJob() override;

    struct ClipDescriptor : private juce::ValueTree::Listener
    {
        ClipDescriptor (ComposedClip& owner, std::shared_ptr<AVClip> clip);

        ClipDescriptor (ComposedClip& owner, juce::ValueTree state);

        juce::String getDescription() const;
        void setDescription (const juce::String& name);

        /** start of the clip in seconds */
        double getStart() const;
        void setStart (double start);

        /** length of the clip in seconds */
        double getLength() const;
        void setLength (double length);

        /** offset in seconds into the media */
        double getOffset() const;
        void setOffset (double offset);

        int getVideoLine() const;
        void setVideoLine (int line);

        int getAudioLine() const;
        void setAudioLine (int line);

        std::shared_ptr<AVClip> clip;

        juce::ValueTree& getStatusTree();

        void updateSampleCounts();

    private:
        std::atomic<int64_t> start {0};
        std::atomic<int64_t> length {0};
        std::atomic<int64_t> offset {0};

        void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                       const juce::Identifier& property) override;

        void valueTreeChildAdded (juce::ValueTree& parentTree,
                                  juce::ValueTree& childWhichHasBeenAdded) override {}

        void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                    juce::ValueTree& childWhichHasBeenRemoved,
                                    int indexFromWhichChildWasRemoved) override {}

        void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                         int oldIndex, int newIndex) override {}

        void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged) override {}

        juce::ValueTree state;
        ComposedClip& owner;

        friend ComposedClip;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipDescriptor)
    };

    juce::ValueTree& getStatusTree();

    std::shared_ptr<ClipDescriptor> addClip (std::shared_ptr<AVClip> clip, double start, double length = -1, double offset = 0);
    void removeClip (std::shared_ptr<ClipDescriptor> descriptor);

    std::vector<std::shared_ptr<ClipDescriptor>> getClips() const;

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;

    void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                     int oldIndex, int newIndex) override {}

    void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged) override {}

private:

    void handleAsyncUpdate() override;

    juce::UndoManager* getUndoManager();

    std::vector<std::shared_ptr<ClipDescriptor>> getActiveClips (std::function<bool(ComposedClip::ClipDescriptor&)> selector) const;

    class ComposingThread : public juce::TimeSliceClient
    {
    public:
        ComposingThread (ComposedClip& owner);
        int useTimeSlice() override;

        void setSuspended (bool s);
        bool isSuspended() const;

    private:
        ComposedClip& owner;
        bool suspended = true;
        bool inRenderBlock = false;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComposingThread)
    };

    juce::CriticalSection clipDescriptorLock;

    VideoFifo videoFifo;
    ComposingThread videoRenderJob;

    juce::ValueTree state;

    std::unique_ptr<CompositingContext> composer;
    std::vector<std::shared_ptr<ClipDescriptor>> clips;
    std::atomic<int64_t> position = {};
    Size videoSize;
    double sampleRate = 0;
    juce::AudioBuffer<float> buffer;
    int64_t lastShownFrame;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComposedClip)
};

} // foleys
