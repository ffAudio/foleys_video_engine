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

class ComposedClip;

/**
 @class ClipDescriptor

 The ClipDescriptor configures the placement of each clip to be used
 in compositing the ComposedClip. It also holds a list of VideoProcessors
 and AudioProcessors including their automation data relative to the clip.
 */
class ClipDescriptor  : public TimeCodeAware,
                        private juce::ValueTree::Listener
{
public:

    class ClipParameterController;

    /**
     Create a ClipDescriptor from an AVClip, that will be included in the ComposedClip.
     Every ClipDescriptor can only live in one ComposedClip.

     @param owner is the ComposedClip, where the Clipdescriptor will live in.
     @param clip the AVClip, that will be wrapped and described by this ClipDescriptor.
     */
    ClipDescriptor (ComposedClip& owner, std::shared_ptr<AVClip> clip, juce::UndoManager* undo);

    /**
     Create a ClipDescriptor from an AVClip, that will be included in the ComposedClip.
     This will use the owners VideoEngine to resolve and load the clip and all processors.
     Every ClipDescriptor can only live in one ComposedClip.

     @param owner is the ComposedClip, where the Clipdescriptor will live in.
     @param state is the ValueTree coded state to describe this ClipDescriptor.
     */
    ClipDescriptor (ComposedClip& owner, juce::ValueTree state, juce::UndoManager* undo);

    ~ClipDescriptor() override;

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
    double getStart() const           { return start; }
    int64_t getStartInSamples() const { return startSamples; }
    void setStart (double start);

    /** length of the clip in seconds */
    double getLength() const           { return length; }
    int64_t getLengthInSamples() const { return lengthSamples; }
    void setLength (double length);

    /** offset in seconds into the media */
    double getOffset() const           { return offset; }
    int64_t getOffsetInSamples() const { return offsetSamples; }
    void setOffset (double offset);

    /** switch video invisible */
    void setVideoVisible (bool shouldBeVisible);
    bool getVideoVisible() const;

    /** switch audio muted */
    void setAudioPlaying (bool shouldPlay);
    bool getAudioPlaying() const;

    /** Transforms a time relative to the containing clip into a local time in ClipDescriptor. */
    double getClipTimeInDescriptorTime (double time) const;

    std::shared_ptr<AVClip> clip;

    /** Read all plugins getStateInformation() and save it into the statusTree as BLOB */
    void readPluginStatesIntoValueTree();

    /** Grants access to the underlying state. Your GUI may use this to add private data.
        It is your responsibility to avoid property or child collissions. */
    juce::ValueTree& getStatusTree();

    void updateSampleCounts();

    ClipParameterController& getAudioParameterController();
    ClipParameterController& getVideoParameterController();

    void addProcessor (juce::ValueTree tree, int index = -1);

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
    };

    /** Add a listener to the ClipDescriptor */
    void addListener (Listener* listener);
    /** Add a listener from the ClipDescriptor */
    void removeListener (Listener* listener);

    /**
     Since the automation values are time dependent, every instance, that inherits
     ControllableBase needs a way to tell the local time (presentation time stamp).
     */
    double getCurrentTimeInSeconds() const override;

    /**
     Sends the current local timecode to it's timecodeListeners.
     */
    void triggerTimecodeUpdate (juce::NotificationType type);

    void updateAudioAutomations (double pts);
    void updateVideoAutomations (double pts);

    //==============================================================================

    class ClipParameterController : public ControllableBase
    {
    public:
        ClipParameterController (TimeCodeAware& timeReference);

        void setClip (const ParameterMap& parameters,
                      juce::ValueTree node,
                      juce::UndoManager* undoManager);

        AutomationMap& getParameters() override;
        int getNumParameters() const override;

        double getValueAtTime (juce::Identifier paramID, double pts, double defaultValue) override;

        double getCurrentPTS() const override;

        void updateAutomations (double pts);

    private:
        AutomationMap parameters;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipParameterController)
    };

private:

    ClipParameterController audioParameterController { *this };
    ClipParameterController videoParameterController { *this };

    std::atomic<double> start  = 0.0;
    std::atomic<double> length  = 0.0;
    std::atomic<double> offset  = 0.0;

    std::atomic<int64_t> startSamples  = 0;
    std::atomic<int64_t> lengthSamples  = 0;
    std::atomic<int64_t> offsetSamples  = 0;

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;

    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}

    void valueTreeParentChanged (juce::ValueTree&) override {}

    ComposedClip&      owner;

    juce::ValueTree    state;
    juce::UndoManager* undoManager = nullptr;
    bool               manualStateChange = false;

    juce::ListenerList<Listener> listeners;

    std::vector<std::unique_ptr<ProcessorController>> videoProcessors;
    std::vector<std::unique_ptr<ProcessorController>> audioProcessors;

    friend ComposedClip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipDescriptor)
};

} // foleys
