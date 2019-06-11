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

class ClipDescriptor;
class ParameterAutomation;


/**
 The ProcessorController acts as container foe one AudioProcessor or VideoProcessor
 inside the ClipDescriptor. It also holds the automation data to update the
 ProcessorParameters according to the currently rendered position.
 */
class ProcessorController  : private juce::ValueTree::Listener
{
public:
    /**
     This ProcessorController constructor creates an instance using a given AudioProcessor.
     It will create all necessary ProcessorAutomation to wrap the AudioProcessorParameters.
     The generated state to represent the controller is added to the state of it's owner.
     */
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<juce::AudioProcessor> processor);

    /**
     This ProcessorController constructor creates an instance using a given VideoProcessor.
     It will create all necessary ProcessorAutomation to wrap the ProcessorParameters.
     The generated state to represent the controller is added to the state of it's owner.
     */
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<VideoProcessor> processor);

    /**
     This ProcessorController constructor creates either an AudioProcessor or VideoProcessor
     from a saved state in a ValueTree.
     */
    ProcessorController (ClipDescriptor& owner, const juce::ValueTree& state, int index=-1);

    ~ProcessorController();

    /** Returns the name of the controlled processor */
    juce::String getName() const;

    /**
     This sets all parameters in the contained processor according to the current
     time point in seconds.
     */
    void updateAutomation (double pts);

    /**
     Calling this method will save the automation of one parameter by discarding
     the ValueTree and recreating it. It has a flag to avoid infinite loops.
     */
    void synchroniseState (ParameterAutomation& parameter);

    /** Calling this method will load the keyframes for an automated parameter. */
    void synchroniseParameter (const juce::ValueTree& tree);

    /** Grants access to the underlying state. Your GUI may use this to add private data.
        It is your responsibility to avoid property or child collissions. */
    juce::ValueTree& getProcessorState();

    ClipDescriptor& getOwningClipDescriptor();

    struct ProcessorAdapter
    {
        ProcessorAdapter() = default;
        virtual ~ProcessorAdapter() = default;

        virtual const juce::String getName() const = 0;
        virtual const juce::String getIdentifierString() const = 0;

        virtual VideoProcessor* getVideoProcessor() { return nullptr; }
        virtual juce::AudioProcessor* getAudioProcessor() { return nullptr; }

        virtual void createAutomatedParameters (ProcessorController& controller,
                                                std::vector<std::unique_ptr<ParameterAutomation>>& parameters,
                                                juce::ValueTree& parameterNode) = 0;
    };

    /** Returns the controlled AudioProcessor. Can be nullptr, if it controlls a
        VideoProcessor or if loading of the plugin failed. */
    juce::AudioProcessor* getAudioProcessor();

    /** Returns the controlled VideoProcessor. Can be nullptr, if it controlls a
        AudioProcessor or if loading of the plugin failed. */
    VideoProcessor* getVideoProcessor();

    std::vector<std::unique_ptr<ParameterAutomation>>& getParameters();

    int getNumParameters() const;

    void setActive (bool shouldBeActive);
    bool isActive() const;

private:

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

    ClipDescriptor& owner;
    juce::ValueTree state;

    bool isAvtive = true;
    bool isUpdating = false;

    std::unique_ptr<ProcessorAdapter> adapter;
    std::vector<std::unique_ptr<ParameterAutomation>> parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorController)
};

} // foleys
