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


namespace foleys
{

//==============================================================================

struct AudioProcessorAdapter : public ProcessorController::ProcessorAdapter
{
    AudioProcessorAdapter (std::unique_ptr<juce::AudioProcessor> p) : processor (std::move (p))
    {
        if (processor.get() != nullptr)
            processor->setPlayHead (&playhead);
    }

    const juce::String getName() const override
    {
        return processor->getName();
    }

    const juce::String getIdentifierString() const override
    {
        if (auto* instance = dynamic_cast<juce::AudioPluginInstance*>(processor.get()))
            return instance->getPluginDescription().createIdentifierString();
        else
            return "BUILTIN: " + processor->getName();
    }

    juce::AudioProcessor* getAudioProcessor() override { return processor.get(); }

    class PlayHead : public juce::AudioPlayHead
    {
    public:
        PlayHead() = default;

        bool getCurrentPosition (juce::AudioPlayHead::CurrentPositionInfo &result) override
        {
            result.timeInSamples = timeInSamples;
            result.timeInSeconds = timeInSeconds;
            result.frameRate = frameRate;
            return true;
        }

        bool canControlTransport() override
        {
            return false;
        }

        juce::int64 timeInSamples = 0;
        double      timeInSeconds = 0.0;
        juce::AudioPlayHead::FrameRateType frameRate = juce::AudioPlayHead::fpsUnknown;


    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayHead)
    };

    void createAutomatedParameters (ProcessorController& controller,
                                    AutomationMap& parameters,
                                    juce::ValueTree& parameterNode,
                                    juce::UndoManager* undoManager) override
    {
        if (processor.get() == nullptr)
            return;

        for (auto parameter : processor->getParameters())
        {
            if (parameter->isAutomatable())
            {
                auto node = parameterNode.getChildWithProperty (IDs::index, parameter->getParameterIndex());
                if (!node.isValid())
                {
                    node = juce::ValueTree { IDs::parameter };
                    node.setProperty (IDs::index, parameter->getParameterIndex(), nullptr);
                    node.setProperty (IDs::name, parameter->getName(128), nullptr);
                    node.setProperty (IDs::value, parameter->getValue(), nullptr);
                    parameterNode.appendChild (node, undoManager);
                }

                if (auto* pWithID = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
                    parameters [juce::Identifier (pWithID->paramID)] = std::make_unique<AudioParameterAutomation> (controller, *pWithID, node, undoManager);
                else
                    parameters [juce::Identifier (juce::String (parameter->getParameterIndex()))] = std::make_unique<AudioParameterAutomation> (controller, *parameter, node, undoManager);
            }
        }
    }

    void setPosition (juce::int64 samples, double seconds) override
    {
        playhead.timeInSamples = samples;
        playhead.timeInSeconds = seconds;
    }

private:
    PlayHead playhead;
    std::unique_ptr<juce::AudioProcessor> processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorAdapter)
};

//==============================================================================

struct VideoProcessorAdapter : public ProcessorController::ProcessorAdapter
{
    VideoProcessorAdapter (std::unique_ptr<VideoProcessor> p) : processor (std::move (p)) {}

    const juce::String getName() const override
    {
        return processor->getName();
    }

    const juce::String getIdentifierString() const override
    {
        return "BUILTIN: " + processor->getName();
    }

    VideoProcessor* getVideoProcessor() override { return processor.get(); }
    std::unique_ptr<VideoProcessor> processor;

    void createAutomatedParameters (ProcessorController& controller,
                                    AutomationMap& parameters,
                                    juce::ValueTree& parameterNode,
                                    juce::UndoManager* undoManager) override
    {
        if (processor.get() == nullptr)
            return;

        for (auto parameter : processor->getParameters())
        {
            auto node = parameterNode.getChildWithProperty (IDs::name, parameter->getName());
            if (!node.isValid())
            {
                node = juce::ValueTree { IDs::parameter };
                node.setProperty (IDs::name, parameter->getName(), undoManager);
                node.setProperty (IDs::value, parameter->getDefaultValue(), undoManager);
                parameterNode.appendChild (node, undoManager);
            }
            parameters [parameter->getParameterID()] = std::make_unique<VideoParameterAutomation> (controller, *parameter, node, undoManager);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoProcessorAdapter)
};

//==============================================================================

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          std::unique_ptr<juce::AudioProcessor> processorToUse)
  : owner (ownerToUse)
{
    auto* undo = owner.getOwningClip().getUndoManager();

    adapter = std::make_unique<AudioProcessorAdapter> (std::move (processorToUse));
    state = juce::ValueTree { IDs::audioProcessor };
    state.setProperty (IDs::name, adapter->getName(), undo);
    state.setProperty (IDs::identifier, adapter->getIdentifierString(), undo);

    adapter->createAutomatedParameters (*this, parameters, state, undo);
}

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          std::unique_ptr<VideoProcessor> processorToUse)
: owner (ownerToUse)
{
    auto* undo = owner.getOwningClip().getUndoManager();

    adapter = std::make_unique<VideoProcessorAdapter> (std::move (processorToUse));
    state = juce::ValueTree { IDs::videoProcessor };
    state.setProperty (IDs::name, adapter->getName(), undo);
    state.setProperty (IDs::identifier, adapter->getIdentifierString(), undo);

    adapter->createAutomatedParameters (*this, parameters, state, undo);
}

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          const juce::ValueTree& stateToUse, int index)
: owner (ownerToUse)
{
    state = stateToUse;

    const auto identifier = state.getProperty (IDs::identifier);
    auto& composedClip = owner.getOwningClip();
    auto* engine = composedClip.getVideoEngine();

    if (engine == nullptr)
    {
        state.setProperty (IDs::pluginStatus, NEEDS_TRANS ("Video engine not present"), nullptr);
        return;
    }

    juce::String error;
    if (state.getType() == IDs::audioProcessor)
    {
        auto processor = engine->createAudioPluginInstance (identifier, composedClip.getSampleRate(), composedClip.getDefaultBufferSize(), error);
        if (processor.get() != nullptr)
        {
            if (state.hasProperty (IDs::state))
            {
                juce::MemoryBlock block;
                if (block.fromBase64Encoding (state.getProperty (IDs::state).toString()))
                {
                    // phew, that's a bit big for the JUCE API
                    jassert (block.getSize() < std::numeric_limits<int>::max());
                    processor->setStateInformation (block.getData(), int (block.getSize()));
                }
            }
            adapter = std::make_unique<AudioProcessorAdapter> (std::move (processor));
            adapter->createAutomatedParameters (*this, parameters, state, owner.getOwningClip().getUndoManager());
        }
    }
    else if (state.getType() == IDs::videoProcessor)
    {
        auto processor = engine->createVideoPluginInstance (identifier, error);
        if (processor.get() != nullptr)
        {
            if (state.hasProperty (IDs::state))
            {
                juce::MemoryBlock block;
                if (block.fromBase64Encoding (state.getProperty (IDs::state).toString()))
                {
                    // phew, that's a bit big for the JUCE API
                    jassert (block.getSize() < std::numeric_limits<int>::max());
                    processor->setStateInformation (block.getData(), int (block.getSize()));
                }
            }
            adapter = std::make_unique<VideoProcessorAdapter> (std::move (processor));
            adapter->createAutomatedParameters (*this, parameters, state, owner.getOwningClip().getUndoManager());
        }
    }
    else
    {
        // you shouldn't feed anything than audioProcessor or videoProcessor states here
        error = NEEDS_TRANS ("Unknown processor type");
        jassertfalse;
    }

    state.setProperty (IDs::pluginStatus, error, nullptr);
}

ProcessorController::~ProcessorController()
{
    if (auto* audioProcessor = adapter->getAudioProcessor())
        audioProcessor->releaseResources();
}

juce::String ProcessorController::getName() const
{
    if (auto* p = adapter->getAudioProcessor())
        return p->getName();

    if (auto* p = adapter->getVideoProcessor())
        return p->getName();

    return NEEDS_TRANS ("Unknown Processor");
}

double ProcessorController::getCurrentPTS() const
{
    return owner.getCurrentPTS();
}

double ProcessorController::getValueAtTime (juce::Identifier paramID, double pts, double defaultValue)
{
    if (auto* parameter = parameters [paramID].get())
        return parameter->getRealValueForTime (pts);

    return defaultValue;
}

void ProcessorController::updateAutomation (double pts)
{
    for (auto& parameter : parameters)
        parameter.second->updateProcessor (pts);
}

juce::ValueTree& ProcessorController::getProcessorState()
{
    return state;
}

void ProcessorController::setActive (bool shouldBeActive)
{
    if (bool (state.getProperty (IDs::active, true)) != shouldBeActive)
    {
        state.setProperty (IDs::active, shouldBeActive, owner.getOwningClip().getUndoManager());
        owner.getOwningClip().invalidateVideo();
    }
}

bool ProcessorController::isActive() const
{
    return state.getProperty (IDs::active, true);
}

void ProcessorController::setPosition (juce::int64 timeInSamples, double timeInSeconds)
{
    adapter->setPosition (timeInSamples, timeInSeconds);
}

AutomationMap& ProcessorController::getParameters()
{
    return parameters;
}

int ProcessorController::getNumParameters() const
{
    return int (parameters.size());
}

ClipDescriptor& ProcessorController::getOwningClipDescriptor()
{
    return owner;
}

juce::AudioProcessor* ProcessorController::getAudioProcessor()
{
    return adapter->getAudioProcessor();
}

VideoProcessor* ProcessorController::getVideoProcessor()
{
    return adapter->getVideoProcessor();
}

void ProcessorController::readPluginStatesIntoValueTree()
{
    juce::MemoryBlock block;
    if (auto* audioProcessor = getAudioProcessor())
    {
        audioProcessor->getStateInformation (block);
    }
    else if (auto* videoProcessor = getVideoProcessor())
    {
        videoProcessor->getStateInformation (block);
    }

    if (block.getSize() > 0)
    {
        auto* undo = owner.getOwningClip().getUndoManager();
        state.setProperty (IDs::state, block.toBase64Encoding(), undo);
    }
}

} // foleys
