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
 The ProcessorController acts as container foe one AudioProcessor or VideoProcessor
 inside the ClipDescriptor. It also holds the automation data to update the
 ProcessorParameters according to the currently rendered position.
 */
class ProcessorController  : public ControllableBase
{
public:
    /**
     This ProcessorController constructor creates an instance using a given AudioProcessor.
     It will create all necessary ProcessorAutomation to wrap the AudioProcessorParameters.
     The generated state to represent the controller is added to the state of it's owner.
     */
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<juce::AudioProcessor> processor, juce::UndoManager* undo);

    /**
     This ProcessorController constructor creates an instance using a given VideoProcessor.
     It will create all necessary ProcessorAutomation to wrap the ProcessorParameters.
     The generated state to represent the controller is added to the state of it's owner.
     */
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<VideoProcessor> processor, juce::UndoManager* undo);

    /**
     This ProcessorController constructor creates either an AudioProcessor or VideoProcessor
     from a saved state in a ValueTree.
     */
    ProcessorController (ClipDescriptor& owner, const juce::ValueTree& state, juce::UndoManager* undo, int index);

    ~ProcessorController();

    /** Returns the name of the controlled processor */
    juce::String getName() const;

    /** Return the current timestamp in seconds of the owning clip */
    double getCurrentPTS() const override;

    double getValueAtTime (juce::Identifier paramID, double pts, double defaultValue) override;

    /**
     This sets all parameters in the contained processor according to the current
     time point in seconds.
     */
    void updateAutomation (double pts);

    /** Read all plugins getStateInformation() and save it into the statusTree as BLOB */
    void readPluginStatesIntoValueTree();

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
                                                AutomationMap& parameters,
                                                juce::ValueTree& parameterNode,
                                                juce::UndoManager* undoManager) = 0;

        virtual void setPosition (juce::int64 timeInSamples, double timeInSeconds)
        {
            juce::ignoreUnused (timeInSamples);
            juce::ignoreUnused (timeInSeconds);
        }
    };

    /** Returns the controlled AudioProcessor. Can be nullptr, if it controlls a
        VideoProcessor or if loading of the plugin failed. */
    juce::AudioProcessor* getAudioProcessor();

    /** Returns the controlled VideoProcessor. Can be nullptr, if it controlls a
        AudioProcessor or if loading of the plugin failed. */
    VideoProcessor* getVideoProcessor();

    AutomationMap& getParameters() override;
    int getNumParameters() const override;

    void setActive (bool shouldBeActive);
    bool isActive() const;

    void setPosition (juce::int64 timeInSamples, double timeInSeconds);

private:

    ClipDescriptor& owner;
    juce::ValueTree state;

    std::unique_ptr<ProcessorAdapter> adapter;
    AutomationMap                     parameters;
    juce::UndoManager*                undoManager=nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorController)
};

} // foleys
