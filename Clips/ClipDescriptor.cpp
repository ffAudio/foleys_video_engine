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

namespace IDs
{
    static juce::Identifier audioProcessor  { "AudioProcessor" };
    static juce::Identifier videoProcessor  { "VideoProcessor" };
    static juce::Identifier identifier      { "Identifier" };
    static juce::Identifier name            { "Name" };
    static juce::Identifier parameter       { "Parameter" };
    static juce::Identifier pluginStatus    { "PluginStatus" };
};

ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, std::shared_ptr<AVClip> clipToUse)
  : owner (ownerToUse)
{
    clip = clipToUse;
    state = juce::ValueTree (IDs::clip);
    auto mediaFile = clip->getMediaFile();
    if (mediaFile.getFullPathName().isNotEmpty())
        state.setProperty (IDs::source, mediaFile.getFullPathName(), nullptr);

    state.getOrCreateChildWithName (IDs::videoProcessors, nullptr);
    state.getOrCreateChildWithName (IDs::audioProcessors, nullptr);

    state.addListener (this);
}

ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, juce::ValueTree stateToUse)
  : owner (ownerToUse)
{
    juce::ScopedValueSetter<bool> manual (manualStateChange, true);

    state = stateToUse;
    auto* engine = owner.getVideoEngine();
    if (state.hasProperty (IDs::source) && engine)
    {
        auto source = state.getProperty (IDs::source);
        clip = engine->createClipFromFile ({ source });

        const auto audioProcessorsNode = state.getOrCreateChildWithName (IDs::audioProcessors, nullptr);
        for (const auto& audioProcessor : audioProcessorsNode)
            addAudioProcessor (std::make_unique<ProcessorController>(*this, audioProcessor));

        const auto videoProcessorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, nullptr);
        for (const auto& videoProcessor : videoProcessorsNode)
            addVideoProcessor (std::make_unique<ProcessorController>(*this, videoProcessor));

    }
    state.addListener (this);
}

juce::String ClipDescriptor::getDescription() const
{
    return state.getProperty (IDs::description, "unnamed");
}

void ClipDescriptor::setDescription (const juce::String& name)
{
    state.setProperty (IDs::description, name, owner.getUndoManager());
}

double ClipDescriptor::getStart() const
{
    return state.getProperty (IDs::start, 0.0);
}

void ClipDescriptor::setStart (double s)
{
    state.setProperty (IDs::start, s, owner.getUndoManager());
}

double ClipDescriptor::getLength() const
{
    return state.getProperty (IDs::length, 0.0);
}

void ClipDescriptor::setLength (double l)
{
    state.setProperty (IDs::length, l, owner.getUndoManager());
}

double ClipDescriptor::getOffset() const
{
    return state.getProperty (IDs::offset, 0.0);
}

void ClipDescriptor::setOffset (double o)
{
    state.setProperty (IDs::offset, o, owner.getUndoManager());
}

double ClipDescriptor::getCurrentPTS() const
{
    return getOwningClip().getCurrentTimeInSeconds() + getOffset() - getStart();
}

int ClipDescriptor::getVideoLine() const
{
    return state.getProperty (IDs::videoLine, 0.0);
}

void ClipDescriptor::setVideoLine (int line)
{
    state.setProperty (IDs::videoLine, line, owner.getUndoManager());
}

int ClipDescriptor::getAudioLine() const
{
    return state.getProperty (IDs::audioLine, 0.0);
}

void ClipDescriptor::setAudioLine (int line)
{
    state.setProperty (IDs::audioLine, line, owner.getUndoManager());
}

void ClipDescriptor::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                             const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != state)
        return;

    updateSampleCounts();
}

void ClipDescriptor::valueTreeChildAdded (juce::ValueTree& parentTree,
                                          juce::ValueTree& childWhichHasBeenAdded)
{
    if (manualStateChange)
        return;

    if (parentTree.getType() == IDs::audioProcessors)
        addAudioProcessor (std::make_unique<ProcessorController>(*this, childWhichHasBeenAdded),
                           parentTree.indexOf (childWhichHasBeenAdded));

    if (parentTree.getType() == IDs::videoProcessors)
        addVideoProcessor (std::make_unique<ProcessorController>(*this, childWhichHasBeenAdded),
                           parentTree.indexOf (childWhichHasBeenAdded));
}

void ClipDescriptor::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                            juce::ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved)
{
    if (manualStateChange)
        return;

    if (parentTree.getType() == IDs::audioProcessors)
        removeAudioProcessor (indexFromWhichChildWasRemoved);

    if (parentTree.getType() == IDs::videoProcessors)
        removeVideoProcessor (indexFromWhichChildWasRemoved);
}

void ClipDescriptor::updateSampleCounts()
{
    auto sampleRate = clip->getSampleRate();

    start = sampleRate * double (state.getProperty (IDs::start));
    length = sampleRate * double (state.getProperty (IDs::length));
    offset = sampleRate * double (state.getProperty (IDs::offset));
}

int64_t ClipDescriptor::getStartInSamples() const { return start; }
int64_t ClipDescriptor::getLengthInSamples() const { return length; }
int64_t ClipDescriptor::getOffsetInSamples() const { return offset; }

juce::ValueTree& ClipDescriptor::getStatusTree()
{
    return state;
}

void ClipDescriptor::addAudioProcessor (std::unique_ptr<ProcessorController> processor, int index)
{
    auto* undo = owner.getUndoManager();

    if (auto* audioProcessor = dynamic_cast<juce::AudioProcessor*> (processor->processor.get()))
        audioProcessor->prepareToPlay (owner.getSampleRate(), owner.getDefaultBufferSize());

    if (manualStateChange == false)
    {
        juce::ScopedValueSetter<bool> manual (manualStateChange, true);
        auto processorsNode = state.getOrCreateChildWithName (IDs::audioProcessors, undo);
        processorsNode.addChild (processor->getProcessorState(), index, undo);
    }

    juce::ScopedLock sl (owner.getCallbackLock());
    if (juce::isPositiveAndBelow (index, audioProcessors.size()))
        audioProcessors.insert (std::next (audioProcessors.begin(), index), std::move (processor));
    else
        audioProcessors.push_back (std::move (processor));
}

void ClipDescriptor::addAudioProcessor (std::unique_ptr<juce::AudioProcessor> processor, int index)
{
    addAudioProcessor (std::make_unique<ProcessorController>(*this, std::move (processor)), index);
}

void ClipDescriptor::removeAudioProcessor (int index)
{
    juce::ScopedLock sl (owner.getCallbackLock());
    audioProcessors.erase (std::next (audioProcessors.begin(), index));
}

void ClipDescriptor::addVideoProcessor (std::unique_ptr<ProcessorController> processor, int index)
{
    auto* undo = owner.getUndoManager();

    if (manualStateChange == false)
    {
        juce::ScopedValueSetter<bool> manual (manualStateChange, true);
        auto processorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, undo);
        processorsNode.addChild (processor->getProcessorState(), index, undo);
    }

    juce::ScopedLock sl (owner.getCallbackLock());
    if (juce::isPositiveAndBelow (index, videoProcessors.size()))
        videoProcessors.insert (std::next (videoProcessors.begin(), index), std::move (processor));
    else
        videoProcessors.push_back (std::move (processor));

}

void ClipDescriptor::addVideoProcessor (std::unique_ptr<VideoProcessor> processor, int index)
{
    addVideoProcessor (std::make_unique<ProcessorController>(*this, std::move (processor)), index);
}

void ClipDescriptor::removeVideoProcessor (int index)
{
    juce::ScopedLock sl (owner.getCallbackLock());
    videoProcessors.erase (std::next (videoProcessors.begin(), index));
}

ComposedClip& ClipDescriptor::getOwningClip()
{
    return owner;
}

const ComposedClip& ClipDescriptor::getOwningClip() const
{
    return owner;
}

//==============================================================================

ClipDescriptor::ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
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

ClipDescriptor::ProcessorController::ProcessorController (ClipDescriptor& ownerToUse,
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

ClipDescriptor::ProcessorController::~ProcessorController()
{
    state.removeListener (this);

    if (auto* audioProcessor = dynamic_cast<juce::AudioProcessor*> (processor.get()))
        audioProcessor->releaseResources();
}

void ClipDescriptor::ProcessorController::updateAutomation (double pts)
{
    for (auto& parameter : parameters)
        parameter->updateProcessor (pts);
}

void ClipDescriptor::ProcessorController::synchroniseState (AutomationParameter& parameter)
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

void ClipDescriptor::ProcessorController::synchroniseParameter (const juce::ValueTree& tree)
{
    if (isUpdating || tree.hasProperty (IDs::name) == false)
        return;

    juce::ScopedValueSetter<bool> updating (isUpdating, true);

    const auto& name = tree.getProperty (IDs::name).toString();
    for (auto& parameter : parameters)
        if (parameter->getName() == name)
            parameter->loadFromValueTree (tree);
}

juce::ValueTree& ClipDescriptor::ProcessorController::getProcessorState()
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

ClipDescriptor& ClipDescriptor::ProcessorController::getOwningClipDescriptor()
{
    return owner;
}

void ClipDescriptor::ProcessorController::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                                    const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged.getType() == IDs::parameter)
        synchroniseParameter (treeWhosePropertyHasChanged);
    else if (treeWhosePropertyHasChanged.getType() == IDs::keyframe)
        synchroniseParameter (treeWhosePropertyHasChanged.getParent());
}

void ClipDescriptor::ProcessorController::valueTreeChildAdded (juce::ValueTree& parentTree,
                                                               juce::ValueTree& childWhichHasBeenAdded)
{
    juce::ignoreUnused (childWhichHasBeenAdded);

    if (parentTree.getType() == IDs::parameter)
        synchroniseParameter (parentTree);
}

void ClipDescriptor::ProcessorController::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                                                 juce::ValueTree& childWhichHasBeenRemoved,
                                                                 int indexFromWhichChildWasRemoved)
{
    juce::ignoreUnused (childWhichHasBeenRemoved);
    juce::ignoreUnused (indexFromWhichChildWasRemoved);

    if (parentTree.getType() == IDs::parameter)
        synchroniseParameter (parentTree);
}

} // foleys
