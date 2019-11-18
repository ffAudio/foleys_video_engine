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
    static juce::Identifier index           { "Index" };
    static juce::Identifier name            { "Name" };
    static juce::Identifier value           { "Value" };
    static juce::Identifier audioParameters { "AudioParameters" };
    static juce::Identifier videoParameters { "VideoParameters" };
    static juce::Identifier parameter       { "Parameter" };
    static juce::Identifier pluginStatus    { "PluginStatus" };
    static juce::Identifier active          { "active" };
};

ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, std::shared_ptr<AVClip> clipToUse, juce::UndoManager* undo)
  : owner (ownerToUse),
    undoManager (undo)
{
    clip = clipToUse;
    state = juce::ValueTree (IDs::clip, {}, {
        juce::ValueTree (IDs::videoParameters),
        juce::ValueTree (IDs::audioParameters),
        juce::ValueTree (IDs::videoProcessors),
        juce::ValueTree (IDs::audioProcessors)
    });

    auto mediaFile = clip->getMediaFile();
    if (mediaFile.toString (true).isNotEmpty())
        state.setProperty (IDs::source, mediaFile.toString (true), nullptr);

    audioParameterController.setClip (clip->getAudioParameters(), state.getOrCreateChildWithName (IDs::audioParameters, nullptr), undoManager);
    videoParameterController.setClip (clip->getVideoParameters(), state.getOrCreateChildWithName (IDs::videoParameters, nullptr), undoManager);

    state.addListener (this);
}

ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, juce::ValueTree stateToUse, juce::UndoManager* undo)
  : owner (ownerToUse),
    undoManager (undo)
{
    juce::ScopedValueSetter<bool> manual (manualStateChange, true);

    state = stateToUse;
    auto* engine = owner.getVideoEngine();
    if (state.hasProperty (IDs::source) && engine)
    {
        auto source = state.getProperty (IDs::source);
        clip = engine->createClipFromFile ({ source });

        audioParameterController.setClip (clip->getAudioParameters(), state.getOrCreateChildWithName (IDs::audioParameters, undoManager), undoManager);
        videoParameterController.setClip (clip->getVideoParameters(), state.getOrCreateChildWithName (IDs::videoParameters, undoManager), undoManager);

        const auto audioProcessorsNode = state.getOrCreateChildWithName (IDs::audioProcessors, undoManager);
        for (const auto& audioProcessor : audioProcessorsNode)
            addAudioProcessor (std::make_unique<ProcessorController>(*this, audioProcessor, undoManager, -1));

        const auto videoProcessorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, undoManager);
        for (const auto& videoProcessor : videoProcessorsNode)
            addVideoProcessor (std::make_unique<ProcessorController>(*this, videoProcessor, undoManager, -1));

    }
    state.addListener (this);
}

ClipDescriptor::~ClipDescriptor()
{
    for (const auto& vp : videoProcessors)
        listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (vp.get()); } );

    for (const auto& ap : audioProcessors)
        listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (ap.get()); } );
}

juce::String ClipDescriptor::getDescription() const
{
    return state.getProperty (IDs::description, "unnamed");
}

void ClipDescriptor::setDescription (const juce::String& name)
{
    state.setProperty (IDs::description, name,undoManager);
}

double ClipDescriptor::getStart() const
{
    return state.getProperty (IDs::start, 0.0);
}

void ClipDescriptor::setStart (double s)
{
    state.setProperty (IDs::start, s, undoManager);
}

double ClipDescriptor::getLength() const
{
    return state.getProperty (IDs::length, 0.0);
}

void ClipDescriptor::setLength (double l)
{
    state.setProperty (IDs::length, l, undoManager);
}

double ClipDescriptor::getOffset() const
{
    return state.getProperty (IDs::offset, 0.0);
}

void ClipDescriptor::setOffset (double o)
{
    state.setProperty (IDs::offset, o, undoManager);
}

void ClipDescriptor::setVideoVisible (bool shouldBeVisible)
{
    state.setProperty (IDs::visible, shouldBeVisible, undoManager);
}

bool ClipDescriptor::getVideoVisible() const
{
    return state.getProperty (IDs::visible, true);
}

void ClipDescriptor::setAudioPlaying (bool shouldPlay)
{
    state.setProperty (IDs::audio, shouldPlay, undoManager);
}

bool ClipDescriptor::getAudioPlaying() const
{
    return state.getProperty (IDs::audio, true);
}

double ClipDescriptor::getCurrentTimeInSeconds() const
{
    return getClipTimeInDescriptorTime (getOwningClip().getCurrentTimeInSeconds());
}

void ClipDescriptor::timecodeChanged (int64_t count, double seconds)
{
    sendTimecode (0, getCurrentTimeInSeconds(), juce::sendNotificationSync);
}

double ClipDescriptor::getClipTimeInDescriptorTime (double time) const
{
    return time + getOffset() - getStart();
}

const std::vector<std::unique_ptr<ProcessorController>>& ClipDescriptor::getVideoProcessors() const
{
    return videoProcessors;
}

const std::vector<std::unique_ptr<ProcessorController>>& ClipDescriptor::getAudioProcessors() const
{
    return audioProcessors;
}

void ClipDescriptor::addListener (Listener* listener)
{
    listeners.add (listener);
}

void ClipDescriptor::removeListener (Listener* listener)
{
    listeners.remove (listener);
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
        addAudioProcessor (std::make_unique<ProcessorController>(*this, childWhichHasBeenAdded, undoManager, -1),
                           parentTree.indexOf (childWhichHasBeenAdded));

    if (parentTree.getType() == IDs::videoProcessors)
        addVideoProcessor (std::make_unique<ProcessorController>(*this, childWhichHasBeenAdded, undoManager, -1),
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

ClipDescriptor::ClipParameterController& ClipDescriptor::getAudioParameterController()
{
    return audioParameterController;
}

ClipDescriptor::ClipParameterController& ClipDescriptor::getVideoParameterController()
{
    return videoParameterController;
}

int64_t ClipDescriptor::getStartInSamples() const { return start; }
int64_t ClipDescriptor::getLengthInSamples() const { return length; }
int64_t ClipDescriptor::getOffsetInSamples() const { return offset; }

juce::ValueTree& ClipDescriptor::getStatusTree()
{
    return state;
}

void ClipDescriptor::addProcessor (juce::ValueTree tree, int index)
{
    if (tree.getType() == IDs::videoProcessor)
    {
        auto node = state.getOrCreateChildWithName (IDs::videoProcessors, undoManager);
        node.addChild (tree, index, undoManager);
    }
    else if (tree.getType() == IDs::audioProcessor)
    {
        auto node = state.getOrCreateChildWithName (IDs::audioProcessors, undoManager);
        node.addChild (tree, index, undoManager);
    }
}

void ClipDescriptor::addAudioProcessor (std::unique_ptr<ProcessorController> controller, int index)
{
    if (auto* audioProcessor = controller->getAudioProcessor())
        audioProcessor->prepareToPlay (owner.getSampleRate(), owner.getDefaultBufferSize());

    if (manualStateChange == false)
    {
        juce::ScopedValueSetter<bool> manual (manualStateChange, true);
        auto processorsNode = state.getOrCreateChildWithName (IDs::audioProcessors, undoManager);
        processorsNode.addChild (controller->getProcessorState(), index, undoManager);
    }

    {
        juce::ScopedLock sl (owner.getCallbackLock());
        if (juce::isPositiveAndBelow (index, audioProcessors.size()))
            audioProcessors.insert (std::next (audioProcessors.begin(), index), std::move (controller));
        else
            audioProcessors.push_back (std::move (controller));
    }

    listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerAdded(); } );
}

void ClipDescriptor::addAudioProcessor (std::unique_ptr<juce::AudioProcessor> processor, int index)
{
    addAudioProcessor (std::make_unique<ProcessorController>(*this, std::move (processor), undoManager), index);
}

void ClipDescriptor::removeAudioProcessor (int index)
{
    const auto& toBeRemoved = std::next (audioProcessors.begin(), index);
    if (toBeRemoved == audioProcessors.end())
        return;

    listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (toBeRemoved->get()); } );

    juce::ScopedLock sl (owner.getCallbackLock());
    audioProcessors.erase (toBeRemoved);
}

void ClipDescriptor::addVideoProcessor (std::unique_ptr<ProcessorController> controller, int index)
{
    if (manualStateChange == false)
    {
        juce::ScopedValueSetter<bool> manual (manualStateChange, true);
        auto processorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, undoManager);
        processorsNode.addChild (controller->getProcessorState(), index, undoManager);
    }

    {
        juce::ScopedLock sl (owner.getCallbackLock());
        if (juce::isPositiveAndBelow (index, videoProcessors.size()))
            videoProcessors.insert (std::next (videoProcessors.begin(), index), std::move (controller));
        else
            videoProcessors.push_back (std::move (controller));
    }

    listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerAdded(); } );
}

void ClipDescriptor::addVideoProcessor (std::unique_ptr<VideoProcessor> processor, int index)
{
    addVideoProcessor (std::make_unique<ProcessorController>(*this, std::move (processor), undoManager), index);
}

void ClipDescriptor::removeVideoProcessor (int index)
{
    const auto& toBeRemoved = std::next (videoProcessors.begin(), index);
    if (toBeRemoved == videoProcessors.end() || manualStateChange)
        return;

    juce::ScopedValueSetter<bool> manual (manualStateChange, true);

    listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (toBeRemoved->get()); } );

    auto processorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, undoManager);
    processorsNode.removeChild (index, undoManager);

    juce::ScopedLock sl (owner.getCallbackLock());
    videoProcessors.erase (toBeRemoved);
}

void ClipDescriptor::removeProcessor (ProcessorController* controller)
{
    const auto& videoToBeRemoved = std::find_if (videoProcessors.begin(),
                                                 videoProcessors.end(),
                                                 [controller](const auto& p){ return p.get() == controller; });
    if (videoToBeRemoved != videoProcessors.end())
    {
        listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (videoToBeRemoved->get()); } );

        auto index = std::distance (videoProcessors.begin(), videoToBeRemoved);
        if (!manualStateChange)
        {
            juce::ScopedValueSetter<bool> manual (manualStateChange, true);
            auto processorsNode = state.getOrCreateChildWithName (IDs::videoProcessors, undoManager);
            processorsNode.removeChild (int (index), undoManager);
        }

        juce::ScopedLock sl (owner.getCallbackLock());
        videoProcessors.erase (videoToBeRemoved);

        return;
    }

    const auto& audioToBeRemoved = std::find_if (audioProcessors.begin(),
                                                 audioProcessors.end(),
                                                 [controller](const auto& p){ return p.get() == controller; });
    if (audioToBeRemoved != audioProcessors.end())
    {
        listeners.call ([&](ClipDescriptor::Listener& l) { l.processorControllerToBeDeleted (audioToBeRemoved->get()); } );

        auto index = std::distance (audioProcessors.begin(), audioToBeRemoved);
        if (!manualStateChange)
        {
            juce::ScopedValueSetter<bool> manual (manualStateChange, true);
            auto processorsNode = state.getOrCreateChildWithName (IDs::audioProcessors, undoManager);
            processorsNode.removeChild (int (index), undoManager);
        }

        juce::ScopedLock sl (owner.getCallbackLock());
        audioProcessors.erase (audioToBeRemoved);
        return;
    }

}

void ClipDescriptor::readPluginStatesIntoValueTree()
{
    for (auto& processor : audioProcessors)
        processor->readPluginStatesIntoValueTree();

    for (auto& processor : videoProcessors)
        processor->readPluginStatesIntoValueTree();
}

void ClipDescriptor::updateAudioAutomations (double pts)
{
    audioParameterController.updateAutomations (pts);
}

void ClipDescriptor::updateVideoAutomations (double pts)
{
    videoParameterController.updateAutomations (pts);
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

ClipDescriptor::ClipParameterController::ClipParameterController (TimeCodeAware& timeReference)
  : ControllableBase (timeReference)
{
}

void ClipDescriptor::ClipParameterController::setClip (const ParameterMap& parametersToConnect,
                                                       juce::ValueTree parameterNode,
                                                       juce::UndoManager* undoManager)
{
    parameters.clear();

    for (auto& parameter : parametersToConnect)
    {
        auto node = parameterNode.getChildWithProperty (IDs::name, parameter.second->getName());
        if (!node.isValid())
        {
            node = juce::ValueTree { IDs::parameter, {
                { IDs::name, parameter.second->getName() },
                { IDs::value, parameter.second->getDefaultValue() }
            }};
            parameterNode.appendChild (node, undoManager);
        }
        parameters [parameter.second->getParameterID()] = std::make_unique<VideoParameterAutomation> (*this, *parameter.second, node, undoManager);
    }
}

AutomationMap& ClipDescriptor::ClipParameterController::getParameters()
{
    return parameters;
}

int ClipDescriptor::ClipParameterController::getNumParameters() const
{
    return int (parameters.size());
}

double ClipDescriptor::ClipParameterController::getCurrentPTS() const
{
    return getTimeReference().getCurrentTimeInSeconds();
}

double ClipDescriptor::ClipParameterController::getValueAtTime (juce::Identifier paramID, double pts, double defaultValue)
{
    if (auto* parameter = parameters [paramID].get())
        return parameter->getRealValueForTime (pts);

    return defaultValue;
}

void ClipDescriptor::ClipParameterController::updateAutomations (double pts)
{
    for (auto& parameter : parameters)
        parameter.second->updateProcessor (pts);
}


} // foleys
