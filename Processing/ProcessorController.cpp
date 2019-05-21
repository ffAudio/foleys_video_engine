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

ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
                                          std::unique_ptr<juce::ControllableProcessorBase> processorToUse)
: owner (ownerToUse)
{
    processor = std::move (processorToUse);

    if (processor.get() != nullptr)
    {
        if (dynamic_cast<juce::AudioProcessor*>(processor.get()) != nullptr)
            state = juce::ValueTree { IDs::audioProcessor };
        else if (dynamic_cast<VideoProcessor*>(processor.get()) != nullptr)
            state = juce::ValueTree { IDs::videoProcessor };
        else
        {
            // You can only put juce::AudioProcessors or VideoProcessors into ProcessorController
            jassertfalse;
        }

        state.setProperty (IDs::name, processor->getName(), nullptr);

        if (auto* instance = dynamic_cast<juce::AudioPluginInstance*>(processor.get()))
            state.setProperty (IDs::identifier,
                               instance->getPluginDescription().createIdentifierString(),
                               nullptr);
        else
            state.setProperty (IDs::identifier, "BUILTIN: " + processor->getName(), nullptr);

        for (auto parameter : processor->getParameters())
            if (parameter->isAutomatable())
                parameters.push_back (std::make_unique<AutomationParameter> (*this, *processor, *parameter));
    }

    for (auto& parameter : parameters)
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

        state.appendChild (automation, nullptr);
    }
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
        processor = engine->createAudioPluginInstance (identifier, composedClip.getSampleRate(), composedClip.getDefaultBufferSize(), error);
    else if (state.getType() == IDs::videoProcessor)
        processor = engine->createVideoPluginInstance (identifier, error);
    else
    {
        // you shouldn't feed anything than audioProcessor or videoProcessor states here
        jassertfalse;
    }

    state.setProperty (IDs::pluginStatus, error, nullptr);

    if (processor.get() == nullptr)
        return;

    for (auto* parameter : processor->getParameters())
    {
        if (!parameter->isAutomatable())
            continue;

        auto& newParameter = parameters.emplace_back (std::make_unique<AutomationParameter> (*this, *processor, *parameter));
        auto node = state.getChildWithProperty (IDs::name, parameter->getName (64));
        if (node.isValid())
            newParameter->loadFromValueTree (node);
    }

    state.addListener (this);
}

ProcessorController::~ProcessorController()
{
    state.removeListener (this);

    if (auto* audioProcessor = dynamic_cast<juce::AudioProcessor*> (processor.get()))
        audioProcessor->releaseResources();
}

void ProcessorController::updateAutomation (double pts)
{
    for (auto& parameter : parameters)
        parameter->updateProcessor (pts);
}

void ProcessorController::synchroniseState (AutomationParameter& parameter)
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

ClipDescriptor& ProcessorController::getOwningClipDescriptor()
{
    return owner;
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

}
