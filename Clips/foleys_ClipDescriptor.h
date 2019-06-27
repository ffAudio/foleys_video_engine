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

class ComposedClip;

/**
 @class ClipDescriptor

 The ClipDescriptor configures the placement of each clip to be used
 in compositing the ComposedClip. It also holds a list of VideoProcessors
 and AudioProcessors including their automation data relative to the clip.
 */
class ClipDescriptor : private juce::ValueTree::Listener
{
public:
    /**
     Create a ClipDescriptor from an AVClip, that will be included in the ComposedClip.
     Every ClipDescriptor can only live in one ComposedClip.

     @param owner is the ComposedClip, where the Clipdescriptor will live in.
     @param clip the AVClip, that will be wrapped and described by this ClipDescriptor.
     */
    ClipDescriptor (ComposedClip& owner, std::shared_ptr<AVClip> clip);

    /**
     Create a ClipDescriptor from an AVClip, that will be included in the ComposedClip.
     This will use the owners VideoEngine to resolve and load the clip and all processors.
     Every ClipDescriptor can only live in one ComposedClip.

     @param owner is the ComposedClip, where the Clipdescriptor will live in.
     @param state is the ValueTree coded state to describe this ClipDescriptor.
     */
    ClipDescriptor (ComposedClip& owner, juce::ValueTree state);

    ~ClipDescriptor();

    /**
     Returns a human readable description of this clip. This is initially set to the
     file name of wrapped clip.
     */
    juce::String getDescription() const;

    /**
     Set the description of this clip.
     */
    void setDescription (const juce::String& name);

    /** start of the clip in seconds */
    double getStart() const;
    int64_t getStartInSamples() const;
    void setStart (double start);

    /** length of the clip in seconds */
    double getLength() const;
    int64_t getLengthInSamples() const;
    void setLength (double length);

    /** offset in seconds into the media */
    double getOffset() const;
    int64_t getOffsetInSamples() const;
    void setOffset (double offset);

    /** switch video invisible */
    void setVideoVisible (bool shouldBeVisible);
    bool getVideoVisible() const;

    /** switch audio muted */
    void setAudioPlaying (bool shouldPlay);
    bool getAudioPlaying() const;

    /** returns the current timestamp in seconds, deduced from audio clock master and
        relative to this clip. */
    double getCurrentPTS() const;

    /** Transforms a time relative to the containing clip into a local time in ClipDescriptor. */
    double getClipTimeInDescriptorTime (double time) const;

    std::shared_ptr<AVClip> clip;

    /** Grants access to the underlying state. Your GUI may use this to add private data.
        It is your responsibility to avoid property or child collissions. */
    juce::ValueTree& getStatusTree();

    void updateSampleCounts();

    void addAudioProcessor (std::unique_ptr<ProcessorController> controller, int index=-1);
    void addAudioProcessor (std::unique_ptr<juce::AudioProcessor> processor, int index=-1);
    void removeAudioProcessor (int index);

    const std::vector<std::unique_ptr<ProcessorController>>& getAudioProcessors() const;

    void addVideoProcessor (std::unique_ptr<ProcessorController> controller, int index=-1);
    void addVideoProcessor (std::unique_ptr<VideoProcessor> processor, int index=-1);
    void removeVideoProcessor (int index);

    void removeProcessor (ProcessorController* controller);

    const std::vector<std::unique_ptr<ProcessorController>>& getVideoProcessors() const;

    ComposedClip& getOwningClip();
    const ComposedClip& getOwningClip() const;

    /**
     Listener to be notified of changes in this ClipDescriptor, especially the removal of
     a ProcessorController.
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void processorControllerAdded() = 0;
        virtual void processorControllerToBeDeleted (const ProcessorController*) = 0;
        virtual void parameterAutomationChanged (const ParameterAutomation*) {}
    };

    /** Add a listener to the ClipDescriptor */
    void addListener (Listener* listener);
    /** Add a listener from the ClipDescriptor */
    void removeListener (Listener* listener);

    void notifyParameterAutomationChange (const ParameterAutomation*);

private:
    std::atomic<int64_t> start {0};
    std::atomic<int64_t> length {0};
    std::atomic<int64_t> offset {0};

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

    juce::ValueTree state;
    bool manualStateChange = false;

    ComposedClip& owner;

    juce::ListenerList<Listener> listeners;

    std::vector<std::unique_ptr<ProcessorController>> videoProcessors;
    std::vector<std::unique_ptr<ProcessorController>> audioProcessors;

    friend ComposedClip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipDescriptor)
};

} // foleys
