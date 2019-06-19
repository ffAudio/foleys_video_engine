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
    AudioProcessorAdapter (std::unique_ptr<juce::AudioProcessor> p) : processor (std::move (p)) {}

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
    std::unique_ptr<juce::AudioProcessor> processor;

    void createAutomatedParameters (ProcessorController& controller,
                                    std::vector<std::unique_ptr<ParameterAutomation>>& parameters,
                                    juce::ValueTree& parameterNode) override
    {
        if (processor.get() == nullptr)
            return;

        for (auto parameter : processor->getParameters())
            if (parameter->isAutomatable())
                parameters.push_back (std::make_unique<AudioParameterAutomation> (controller, *parameter));

        for (auto& parameter : parameters)
        {
            auto node = parameterNode.getChildWithProperty (IDs::name, parameter->getName());
            if (node.isValid())
                parameter->loadFromValueTree (node);
            else
            {
                juce::ValueTree automation { IDs::parameter };
                automation.setProperty (IDs::name, parameter->getName(), nullptr);
                automation.setProperty (IDs::value, parameter->getValue(), nullptr);

                for (const auto& key : parameter->getKeyframes())
                {
                    juce::ValueTree keyframeNode { IDs::keyframe };
                    keyframeNode.setProperty (IDs::time, key.first, nullptr);
                    keyframeNode.setProperty (IDs::value, key.second, nullptr);
                    automation.appendChild ( keyframeNode, nullptr);
                }
                parameterNode.appendChild (automation, nullptr);
            }
        }

    }

};

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
                                    std::vector<std::unique_ptr<ParameterAutomation>>& parameters,
                                    juce::ValueTree& parameterNode) override
    {
        if (processor.get() == nullptr)
            return;

        for (auto parameter : processor->getParameters())
            parameters.push_back (std::make_unique<VideoParameterAutomation> (controller, *parameter));

        for (auto& parameter : parameters)
        {
            auto node = parameterNode.getChildWithProperty (IDs::name, parameter->getName());
            if (node.isValid())
                parameter->loadFromValueTree (node);
            else
            {
                juce::ValueTree automation { IDs::parameter };
                automation.setProperty (IDs::name, parameter->getName(), nullptr);
                automation.setProperty (IDs::value, parameter->getValue(), nullptr);

                for (const auto& key : parameter->getKeyframes())
                {
                    juce::ValueTree keyframeNode { IDs::keyframe };
                    keyframeNode.setProperty (IDs::time, key.first, nullptr);
                    keyframeNode.setProperty (IDs::value, key.second, nullptr);
                    automation.appendChild ( keyframeNode, nullptr);
                }
                parameterNode.appendChild (automation, nullptr);
            }
        }

    }

};

//==============================================================================

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          std::unique_ptr<juce::AudioProcessor> processorToUse)
: owner (ownerToUse)
{
    adapter = std::make_unique<AudioProcessorAdapter> (std::move (processorToUse));
    state = juce::ValueTree { IDs::audioProcessor };
    state.setProperty (IDs::name, adapter->getName(), nullptr);
    state.setProperty (IDs::identifier, adapter->getIdentifierString(), nullptr);

    adapter->createAutomatedParameters (*this, parameters, state);
}

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          std::unique_ptr<VideoProcessor> processorToUse)
: owner (ownerToUse)
{
    adapter = std::make_unique<VideoProcessorAdapter> (std::move (processorToUse));
    state = juce::ValueTree { IDs::videoProcessor };
    state.setProperty (IDs::name, adapter->getName(), nullptr);
    state.setProperty (IDs::identifier, adapter->getIdentifierString(), nullptr);

    adapter->createAutomatedParameters (*this, parameters, state);
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
            adapter = std::make_unique<AudioProcessorAdapter> (std::move (processor));
            adapter->createAutomatedParameters (*this, parameters, state);
        }
    }
    else if (state.getType() == IDs::videoProcessor)
    {
        auto processor = engine->createVideoPluginInstance (identifier, error);
        if (processor.get() != nullptr)
        {
            adapter = std::make_unique<VideoProcessorAdapter> (std::move (processor));
            adapter->createAutomatedParameters (*this, parameters, state);
        }
    }
    else
    {
        // you shouldn't feed anything than audioProcessor or videoProcessor states here
        error = NEEDS_TRANS ("Unknown processor type");
        jassertfalse;
    }

    state.setProperty (IDs::pluginStatus, error, nullptr);

    state.addListener (this);
}

ProcessorController::~ProcessorController()
{
    state.removeListener (this);

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

void ProcessorController::updateAutomation (double pts)
{
    for (auto& parameter : parameters)
        parameter->updateProcessor (pts);
}

void ProcessorController::synchroniseState (ParameterAutomation& parameter)
{
    if (isUpdating)
        return;

    juce::ScopedValueSetter<bool> updating (isUpdating, true);

    auto* undo = getOwningClipDescriptor().getOwningClip().getUndoManager();

    auto node = state.getChildWithProperty (IDs::name, parameter.getName());
    if (!node.isValid())
    {
        node = juce::ValueTree { IDs::parameter };
        node.setProperty (IDs::name, parameter.getName(), undo);
        state.appendChild (node, undo);
    }
    parameter.saveToValueTree (node, undo);
}

void ProcessorController::synchroniseParameter (const juce::ValueTree& tree)
{
    if (isUpdating || tree.hasProperty (IDs::name) == false)
        return;

    juce::ScopedValueSetter<bool> updating (isUpdating, true);

    const auto& name = tree.getProperty (IDs::name).toString();
    for (auto& parameter : parameters)
        if (parameter->getName() == name)
            parameter->loadFromValueTree (tree);
}

juce::ValueTree& ProcessorController::getProcessorState()
{
    auto* undo = owner.getOwningClip().getUndoManager();

    for (auto& parameter : parameters)
    {
        auto node = state.getChildWithProperty (IDs::name, parameter->getName());
        if (node.isValid())
        {
            parameter->saveToValueTree (node, undo);
        }
        else
        {
            juce::ValueTree child { IDs::parameter };
            child.setProperty (IDs::name, parameter->getName(), undo);
            state.appendChild (child, undo);
            parameter->saveToValueTree (child, undo);
        }
    }

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

std::vector<std::unique_ptr<ParameterAutomation>>& ProcessorController::getParameters()
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

void ProcessorController::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                    const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged.getType() == IDs::parameter)
        synchroniseParameter (treeWhosePropertyHasChanged);
    else if (treeWhosePropertyHasChanged.getType() == IDs::keyframe)
        synchroniseParameter (treeWhosePropertyHasChanged.getParent());
}

void ProcessorController::valueTreeChildAdded (juce::ValueTree& parentTree,
                                               juce::ValueTree& childWhichHasBeenAdded)
{
    juce::ignoreUnused (childWhichHasBeenAdded);

    if (parentTree.getType() == IDs::parameter)
        synchroniseParameter (parentTree);
}

void ProcessorController::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                                 juce::ValueTree& childWhichHasBeenRemoved,
                                                 int indexFromWhichChildWasRemoved)
{
    juce::ignoreUnused (childWhichHasBeenRemoved);
    juce::ignoreUnused (indexFromWhichChildWasRemoved);

    if (parentTree.getType() == IDs::parameter)
        synchroniseParameter (parentTree);
}

} // foleys
