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
 The ControllableBase acts as counterpart to ParameterAutomation. By inheriting this
 interface, it is possible to automate things like AudioProcessors or VideoProcessors.
 But also Clips could have automated parameters, e.g. for geometric information, alpha
 or an audio gain.
 */
class ControllableBase
{
public:
    ControllableBase() = default;
    virtual ~ControllableBase() = default;

    /**
     Since the automation values are time dependent, every instance, that inherits
     ControllableBase needs a way to tell the local time (presentation time stamp).
     */
    virtual double getCurrentPTS() const = 0;

    /**
     Grant access to the individual parameters.
     */
    virtual std::vector<std::unique_ptr<ParameterAutomation>>& getParameters() = 0;
    virtual int getNumParameters() const = 0;

    /**
     This notifies all ProcessorController::Listeners about an automation change,
     so they can adapt accordingly by redrawing the curves or invalidating pre-rendered
     video frames.
     */
    virtual void notifyParameterAutomationChange (const ParameterAutomation*) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllableBase)
};

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

    /** Return the current timestamp in seconds of the owning clip */
    double getCurrentPTS() const override;

    /**
     This sets all parameters in the contained processor according to the current
     time point in seconds.
     */
    void updateAutomation (double pts);

    void notifyParameterAutomationChange (const ParameterAutomation*) override;

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

    std::vector<std::unique_ptr<ParameterAutomation>>& getParameters() override;
    int getNumParameters() const override;

    void setActive (bool shouldBeActive);
    bool isActive() const;

    void setPosition (juce::int64 timeInSamples, double timeInSeconds);

private:

    ClipDescriptor& owner;
    juce::ValueTree state;

    bool isUpdating = false;

    std::unique_ptr<ProcessorAdapter> adapter;
    std::vector<std::unique_ptr<ParameterAutomation>> parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorController)
};

} // foleys
