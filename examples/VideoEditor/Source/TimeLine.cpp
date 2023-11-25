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

    TimeLine.cpp
    Created: 30 Mar 2019 4:46:00pm
    Author:  Daniel Walz

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include "Player.h"
#include "Properties.h"
#include "TimeLine.h"

namespace IDs
{
    static Identifier videoLine { "videoLine" };
    static Identifier audioLine { "audioLine" };
}

//==============================================================================
TimeLine::TimeLine (foleys::VideoEngine& theVideoEngine, Player& playerToUse, Properties& properiesToUse)
  : videoEngine (theVideoEngine),
    player (playerToUse),
    properties (properiesToUse)
{
    setWantsKeyboardFocus (true);

    addAndMakeVisible (timemarker);
    timemarker.setAlwaysOnTop (true);
}

TimeLine::~TimeLine()
{
    if (edit)
    {
        edit->removeTimecodeListener (this);
        edit->getStatusTree().removeListener (this);
    }

    edit = nullptr;
}

void TimeLine::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());

    g.setColour (Colours::darkgrey);
    for (int i=0; i < numVideoLines; ++i)
        g.fillRect (0, margin + i * (videoHeight + margin), getWidth(), videoHeight);

    g.setColour (Colours::darkgrey.darker());
    for (int i=0; i < numAudioLines; ++i)
        g.fillRect (0, numVideoLines * (videoHeight + margin) + margin + i * (audioHeight + margin), getWidth(), audioHeight);
}

void TimeLine::resized()
{
    if (edit == nullptr)
        return;

    auto sampleRate = getSampleRate();
    if (sampleRate == 0)
        sampleRate = 48000.0;

    timelineLength = std::max (60.0, edit->getLengthInSeconds() * 1.1);

    for (auto& component : clipComponents)
    {
        if (component->isVideoClip())
        {
            int videoline = component->clip->getStatusTree().getProperty (IDs::videoLine, 0);
            component->setBounds (getXFromTime (component->clip->getStart()), margin + videoline * (videoHeight + margin),
                                  getXFromTime (component->clip->getLength()), videoHeight);
        }
        else
        {
            int audioline = component->clip->getStatusTree().getProperty (IDs::audioLine, 0);
            component->setBounds (getXFromTime (component->clip->getStart()), numVideoLines * (videoHeight + margin) + margin + audioline * (audioHeight + margin),
                                  getXFromTime (component->clip->getLength()), audioHeight);
        }
    }

    auto tx = getXFromTime (player.getCurrentTimeInSeconds());
    timemarker.setBounds (tx, 0, 3, getHeight());
}

void TimeLine::timecodeChanged (int64_t, double seconds)
{
    ignoreUnused (time);
    auto tx = getXFromTime (seconds);
    timemarker.setBounds (tx, 0, 3, getHeight());
}

void TimeLine::mouseDown (const MouseEvent& event)
{
    player.setPosition (getTimeFromX (event.x));
}

bool TimeLine::keyPressed (const KeyPress& key)
{
    if (key.isKeyCode (KeyPress::leftKey))
    {
        player.previousFrame();
        return true;
    }

    if (key.isKeyCode (KeyPress::rightKey))
    {
        player.nextFrame();
        return true;
    }

    return false;
}

bool TimeLine::isInterestedInFileDrag (const StringArray&)
{
    return edit != nullptr;
}

void TimeLine::filesDropped (const StringArray& files, int x, int y)
{
    if (files.isEmpty() || edit == nullptr)
        return;

    auto clip = videoEngine.createClipFromFile (juce::URL (juce::File (files [0])));
    if (clip.get() != nullptr)
        addClipToEdit (clip, getTimeFromX (x), y);
}

bool TimeLine::isInterestedInDragSource (const SourceDetails &dragSourceDetails)
{
    if (edit == nullptr)
        return false;

    return (dragSourceDetails.description == "media" || juce::URL (dragSourceDetails.description.toString()).isWellFormed());
}

void TimeLine::itemDropped (const SourceDetails &dragSourceDetails)
{
    if (edit == nullptr)
        return;

    if (auto* source = dynamic_cast<FileTreeComponent*> (dragSourceDetails.sourceComponent.get()))
    {
        auto clip = videoEngine.createClipFromFile (URL (source->getSelectedFile()));

        if (clip.get() == nullptr)
        {
            AlertWindow::showNativeDialogBox (NEEDS_TRANS ("Loading failed"), NEEDS_TRANS (""), true);
            return;
        }

        addClipToEdit (clip, getTimeFromX (dragSourceDetails.localPosition.x), dragSourceDetails.localPosition.y);
        return;
    }

    auto url = dragSourceDetails.description.toString();

    auto clip = videoEngine.createClipFromFile (juce::URL (url));
    if (clip.get() != nullptr)
        addClipToEdit (clip, getTimeFromX (dragSourceDetails.localPosition.x), dragSourceDetails.localPosition.y);
}

bool TimeLine::isInterestedInTextDrag (const String& text)
{
    juce::URL url (text);
    return url.isWellFormed();
}

void TimeLine::textDropped (const String& text, int x, int y)
{
    if (edit.get() == nullptr)
        return;

    auto clip = videoEngine.createClipFromFile (juce::URL (text));
    if (clip.get() != nullptr)
        addClipToEdit (clip, getTimeFromX (x), y);
}

void TimeLine::addClipToEdit (std::shared_ptr<foleys::AVClip> clip, double start, int y)
{
    auto length = -1.0;

    if (std::dynamic_pointer_cast<foleys::ImageClip>(clip) != nullptr)
    {
        length = 3.0;
    }
    else if (clip->hasVideo() == false)
    {
        const auto editLength = edit->getLengthInSeconds();
        length = editLength > start ? std::min (editLength - start, 60.0) : 60.0;
    }

    foleys::ComposedClip::ClipPosition position;
    position.start = start;
    position.length = length;
    auto descriptor = edit->addClip (clip, position);
    if (y < 190)
    {
        int line = juce::roundToInt ((y - margin) / double (videoHeight + margin));
        setVideoLine (descriptor, line);
    }
    else
    {
        int line = (y - numVideoLines * (videoHeight + margin) + margin) / (audioHeight + margin);
        setAudioLine (descriptor, line);
    }

    restoreClipComponents();

    setSelectedClip (descriptor, descriptor->clip->hasVideo());
    resized();
}

void TimeLine::setSelectedClip (std::shared_ptr<foleys::ClipDescriptor> clip, bool video)
{
    selectedClip = clip;
    selectedIsVideo = video;
    properties.showClipProperties (videoEngine, clip, player, video);
    repaint();
}

std::shared_ptr<foleys::ClipDescriptor> TimeLine::getSelectedClip() const
{
    return selectedClip.lock();
}

bool TimeLine::selectedClipIsVideo() const
{
    return selectedIsVideo;
}

void TimeLine::toggleVisibility()
{
    auto clip = selectedClip.lock();
    if (clip.get() == nullptr)
        return;

    if (selectedIsVideo)
        clip->setVideoVisible (! clip->getVideoVisible());
    else
        clip->setAudioPlaying (! clip->getAudioPlaying());

    repaint();
}

void TimeLine::spliceSelectedClipAtPlayPosition()
{
    spliceSelectedClipAtPosition (player.getCurrentTimeInSeconds());
}

void TimeLine::spliceSelectedClipAtPosition (double pts)
{
    auto clip = selectedClip.lock();
    if (clip.get() == nullptr)
        return;

    auto start  = clip->getStart();
    auto length = clip->getLength();
    auto offset = clip->getOffset() + (pts - start);

    if (pts < start || pts > start + length)
        return;

    edit->getStatusTree().addChild (clip->getStatusTree().createCopy(), -1, videoEngine.getUndoManager());
    clip->setLength (pts - clip->getStart());
    clip->updateSampleCounts();

    auto newClip = edit->getClip (int (edit->getClips().size()) - 1);
    if (newClip.get() == nullptr)
        return;

    newClip->setStart (pts);
    newClip->setLength (length - (pts - start));
    newClip->setOffset (offset);

    newClip->setDescription (edit->makeUniqueDescription(clip->getDescription()));
    newClip->updateSampleCounts();
}

void TimeLine::restoreClipComponents()
{
    if (edit == nullptr)
        return;

    auto clips = edit->getClips();
    for (auto descriptor : clips)
    {
        if (descriptor->clip->hasVideo())
        {
            auto comp = std::find_if (clipComponents.begin(), clipComponents.end(), [descriptor](const auto& c){ return c->isVideoClip() && c->clip == descriptor; });
            if (comp == clipComponents.end())
                addClipComponent (descriptor, true);
        }

        if (descriptor->clip->hasAudio())
        {
            auto comp = std::find_if (clipComponents.begin(), clipComponents.end(), [descriptor](const auto& c){ return !c->isVideoClip() && c->clip == descriptor; });
            if (comp == clipComponents.end())
                addClipComponent (descriptor, false);
        }
    }

    clipComponents.erase (std::remove_if (clipComponents.begin(),
                                          clipComponents.end(),
                                          [&](auto& component)
                                          {
                                              auto findClipWithComponent = [&](const auto& clip){ return clip == component->clip; };
                                              return std::find_if (clips.begin(), clips.end(), findClipWithComponent) == clips.end();
                                          }), clipComponents.end());

    resized();
}

void TimeLine::addClipComponent (std::shared_ptr<foleys::ClipDescriptor> clip, bool video)
{
    auto strip = std::make_unique<ClipComponent> (*this, clip, videoEngine.getThreadPool(), video);
    addAndMakeVisible (strip.get());
    clipComponents.emplace_back (std::move (strip));
}

void TimeLine::setEditClip (std::shared_ptr<foleys::ComposedClip> clip)
{
    if (edit)
    {
        edit->removeTimecodeListener (this);
        edit->getStatusTree().removeListener (this);
    }

    edit = clip;

    if (edit)
    {
        edit->getStatusTree().addListener (this);
        edit->addTimecodeListener (this);
    }

    restoreClipComponents();

    player.setClip (edit, true);
}

std::shared_ptr<foleys::ComposedClip> TimeLine::getEditClip() const
{
    return edit;
}

int TimeLine::getXFromTime (double seconds) const
{
    return juce::roundToInt ((seconds / timelineLength) * getWidth());
}

double TimeLine::getTimeFromX (int pixels) const
{
    auto w = getWidth();
    return w > 0 ? timelineLength * pixels / w : 0;
}

int TimeLine::getVideoLine (const std::shared_ptr<foleys::ClipDescriptor> descriptor) const
{
    return descriptor->getStatusTree().getProperty (IDs::videoLine, 0);
}

int TimeLine::getAudioLine (const std::shared_ptr<foleys::ClipDescriptor> descriptor) const
{
    return descriptor->getStatusTree().getProperty (IDs::audioLine, 0);
}

void TimeLine::setVideoLine (std::shared_ptr<foleys::ClipDescriptor> descriptor, int lane) const
{
    descriptor->getStatusTree().setProperty (IDs::videoLine,
                                             jlimit (0, numVideoLines - 1, lane),
                                             videoEngine.getUndoManager());
}

void TimeLine::setAudioLine (std::shared_ptr<foleys::ClipDescriptor> descriptor, int lane) const
{
    descriptor->getStatusTree().setProperty (IDs::audioLine,
                                             jlimit (0, numAudioLines - 1, lane),
                                             videoEngine.getUndoManager());
}

double TimeLine::getSampleRate() const
{
    return player.getSampleRate();
}

void TimeLine::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    MessageManager::callAsync ([safeComponent = SafePointer<TimeLine> (this)] () mutable { if (safeComponent) safeComponent->resized(); });
}

void TimeLine::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    MessageManager::callAsync ([safeComponent = SafePointer<TimeLine> (this)] () mutable { if (safeComponent) safeComponent->restoreClipComponents(); });
}

void TimeLine::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int)
{
    MessageManager::callAsync ([safeComponent = SafePointer<TimeLine> (this)] () mutable { if (safeComponent) safeComponent->restoreClipComponents(); });
}

//==============================================================================

TimeLine::ClipComponent::ClipComponent (TimeLine& tl,
                                        std::shared_ptr<foleys::ClipDescriptor> clipToUse,
                                        ThreadPool&, bool video)
  : clip (clipToUse),
    timeline (tl)
{
    setWantsKeyboardFocus (true);

    if (video)
    {
        filmstrip = std::make_unique<foleys::FilmStrip>();
        filmstrip->setClip (clip->clip);
        addAndMakeVisible (filmstrip.get());
    }
    else
    {
        audiostrip = std::make_unique<foleys::AudioStrip>();
        audiostrip->setClip (clip->clip);
        addAndMakeVisible (audiostrip.get());
    }

    updateProcessorList();
    processorSelect.setColour (ComboBox::backgroundColourId, Colours::black.withAlpha (0.2f));
    addAndMakeVisible (processorSelect);
    processorSelect.onChange = [&]
    {
        if (processorSelect.getSelectedId() == 1)
        {
            if (isVideoClip())
                updateParameterGraphs (clip->getVideoParameterController());
            else
                updateParameterGraphs (clip->getAudioParameterController());
        }
        else
        {
            auto index = processorSelect.getSelectedId()-10;

            if (isVideoClip())
            {
                if (isPositiveAndBelow (index, clip->getVideoProcessors().size()))
                    updateParameterGraphs (*clip->getVideoProcessors() [size_t(index)]);
            }
            else
            {
                if (isPositiveAndBelow (index, clip->getAudioProcessors().size()))
                    updateParameterGraphs (*clip->getAudioProcessors() [size_t(index)]);
            }
        }
    };

    clip->addListener (this);
}

TimeLine::ClipComponent::~ClipComponent()
{
    if (clip.get() != nullptr)
        clip->removeListener (this);
}

void TimeLine::ClipComponent::paint (Graphics& g)
{
    bool selected = timeline.getSelectedClip() == clip;

    g.fillAll (Colours::darkgrey);

    auto colour = isVideoClip() ? Colours::orange : Colours::darkgreen;

    g.setColour (highlight ? Colours::red : (selected ? colour : colour.darker()));
    g.fillRoundedRectangle (getLocalBounds().reduced (1).toFloat(), 5.0);

    if (selected)
        g.setColour (timeline.selectedIsVideo == isVideoClip() ? colour.contrasting() : colour.contrasting().withAlpha (0.5f));
    else
        g.setColour (colour);

    g.drawRoundedRectangle (getLocalBounds().toFloat(), 5.0, 2.0);
    if (clip == nullptr)
        return;

    g.drawFittedText (clip->getDescription(), 5, 3, 180, 18, Justification::left, 1);

    if (filmstrip.get() != nullptr)
        filmstrip->setAlpha (clip->getVideoVisible() ? 1.0f : 0.5f);

    if (audiostrip.get() != nullptr)
        audiostrip->setAlpha (clip->getAudioPlaying() ? 1.0f : 0.3f);
}

void TimeLine::ClipComponent::resized()
{
    if (clip == nullptr)
        return;

    processorSelect.setBounds (190, 2, 160, 18);
    if (filmstrip)
    {
        filmstrip->setBounds (1, 20, getWidth() - 2, getHeight() - 25);
        filmstrip->setStartAndEnd (getLeftTime(), getRightTime());
    }
    if (audiostrip)
    {
        audiostrip->setBounds (1, 20, getWidth() - 2, getHeight() - 25);
        audiostrip->setStartAndEnd (getLeftTime(), getRightTime());
    }

    for (auto& graph : automations)
        graph->setBounds (1, 20, getWidth() - 2, getHeight() - 25);
}

double TimeLine::ClipComponent::getLeftTime() const
{
    if (clip != nullptr)
        return clip->getOffset();

    return 0;
}

double TimeLine::ClipComponent::getRightTime() const
{
    if (clip != nullptr)
        return clip->getLength() + clip->getOffset();

    return 0;
}

void TimeLine::ClipComponent::mouseMove (const MouseEvent& event)
{
    if (event.x > getWidth() - 5 || event.x < 5)
        setMouseCursor (MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor (MouseCursor::DraggingHandCursor);
}

void TimeLine::ClipComponent::mouseDown (const MouseEvent& event)
{
    wasSelected = timeline.getSelectedClip() == clip;

    localDragStart = event.getPosition();
    timeline.setSelectedClip (clip, isVideoClip());

    if (event.x < 5)
        dragmode = dragOffset;
    else if (event.x > getWidth() - 5)
        dragmode = dragLength;
    else
        dragmode = dragPosition;
}

void TimeLine::ClipComponent::mouseDrag (const MouseEvent& event)
{
    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    if (dragmode == dragPosition)
        clip->setStart (std::max (timeline.getTimeFromX ((event.x - localDragStart.x) + getX()), 0.0));
    else if (dragmode == dragLength)
        clip->setLength (std::min (timeline.getTimeFromX (event.x), clip->clip->getLengthInSeconds()));
    else if (dragmode == dragOffset)
    {
        auto oldPosition = clip->getStart();
        auto oldOffset   = clip->getOffset();
        auto oldLength   = clip->getLength();
        auto delta = std::min (std::max (timeline.getTimeFromX ((event.x - localDragStart.x) + getX()) - oldPosition,
                                         -oldOffset), oldLength);
        clip->setStart (oldPosition + delta);
        clip->setOffset (oldOffset + delta);
        clip->setLength (oldLength - delta);
        clip->updateSampleCounts();
    }

    if (filmstrip)
    {
        int line = (event.y + getY() - timeline.margin) / (timeline.videoHeight + timeline.margin);
        if (line != timeline.getVideoLine (clip))
            timeline.setVideoLine (clip, line);
    }
    else
    {
        int line = (event.y + getY() - (timeline.numVideoLines * (timeline.videoHeight + timeline.margin) + timeline.margin)) / (timeline.videoHeight + timeline.margin);
        if (line != timeline.getAudioLine (clip))
            timeline.setAudioLine (clip, line);
    }

    parent->resized();
}

void TimeLine::ClipComponent::mouseUp (const MouseEvent& event)
{
    dragmode = notDragging;

    if (event.mouseWasDraggedSinceMouseDown() == false && wasSelected)
        timeline.player.setPosition (timeline.getTimeFromX (timeline.getLocalPoint (this, event.getPosition()).getX()));
}

bool TimeLine::ClipComponent::keyPressed (const KeyPress& key)
{
    return timeline.keyPressed (key);
}

bool TimeLine::ClipComponent::isInterestedInDragSource (const SourceDetails &dragSourceDetails)
{
    auto tree = ValueTree::fromXml (dragSourceDetails.description.toString());
    if (!tree.isValid())
        return false;

    if (isVideoClip())
        return tree.getType().toString() == "VideoProcessor";
    else
        return tree.getType().toString() == "AudioProcessor";
}

void TimeLine::ClipComponent::itemDragEnter (const SourceDetails &dragSourceDetails)
{
    highlight = isInterestedInDragSource (dragSourceDetails);
    repaint();
}

void TimeLine::ClipComponent::itemDragExit (const SourceDetails&)
{
    highlight = false;
    repaint();
}

void TimeLine::ClipComponent::itemDropped (const SourceDetails &dragSourceDetails)
{
    highlight = false;
    auto tree = ValueTree::fromXml (dragSourceDetails.description.toString());
    if (!tree.isValid() || clip == nullptr)
        return;

    clip->addProcessor (tree);
}

void TimeLine::ClipComponent::updateProcessorList()
{
    auto current = processorSelect.getText();
    processorSelect.clear();
    automations.clear();

    if (isVideoClip())
    {
        processorSelect.addItem (clip->clip->getClipType(), 1);

        int index = 10;
        for (auto& processor : clip->getVideoProcessors())
        {
            if (processor.get() == nullptr)
                continue;

            const auto name = processor->getName();
            processorSelect.addItem (name, index);
            if (name == current)
                processorSelect.setSelectedId (index, sendNotification);

            ++index;
        }
    }
    else
    {
        processorSelect.addItem (clip->clip->getClipType(), 1);

        int index = 10;
        for (auto& processor : clip->getAudioProcessors())
        {
            if (processor.get() == nullptr)
                continue;

            const auto name = processor->getName();
            processorSelect.addItem (name, index);
            if (name == current)
                processorSelect.setSelectedId (index, sendNotification);

            ++index;
        }
    }

    if (current.isEmpty())
        processorSelect.setSelectedId (1);
}

void TimeLine::ClipComponent::updateParameterGraphs (foleys::ControllableBase& controller)
{
    automations.clear();
    for (const auto& parameter : controller.getParameters())
    {
        auto colour = juce::Colour::fromString (parameter.second->getParameterProperties().getWithDefault("Colour", "ffA0A0A0").toString());
        auto graph = std::make_unique<ParameterGraph>(*this, *parameter.second);
        graph->setColour (colour);
        addAndMakeVisible (graph.get());
        automations.push_back (std::move (graph));
    }
    resized();
}

bool TimeLine::ClipComponent::isVideoClip() const
{
    return filmstrip.get() != nullptr;
}

void TimeLine::ClipComponent::processorControllerAdded()
{
    updateProcessorList();
}

void TimeLine::ClipComponent::processorControllerToBeDeleted (const foleys::ProcessorController*)
{
    updateProcessorList();
}

//==============================================================================

TimeLine::ClipComponent::ParameterGraph::ParameterGraph (ClipComponent& ownerToUse,
                                                         foleys::ParameterAutomation& automationToUse)
  : owner (ownerToUse),
    automation (automationToUse)
{
    setOpaque (false);
    automation.getControllable().addListener (this);
}

TimeLine::ClipComponent::ParameterGraph::~ParameterGraph()
{
    automation.getControllable().removeListener (this);
}

void TimeLine::ClipComponent::ParameterGraph::setColour (juce::Colour colourToUse)
{
    colour = colourToUse;
    repaint();
}

void TimeLine::ClipComponent::ParameterGraph::paint (Graphics& g)
{
    g.setColour (colour);
    auto lastX = 1;
    auto lastY = mapFromValue (automation.getValueForTime (owner.getLeftTime()));

    for (const auto& it : automation.getKeyframes())
    {
        auto nextX = mapFromTime (it.first);
        auto nextY = mapFromValue (it.second);
        g.drawLine (lastX, lastY, nextX, nextY, 2.0f);
        g.fillEllipse (nextX - 3, nextY - 3, 7, 7);
        lastX = nextX;
        lastY = nextY;
    }

    const auto end = owner.getRightTime();
    g.drawLine (lastX, lastY, mapFromTime (end), mapFromValue (automation.getValueForTime (end)), 2.0f);
}

void TimeLine::ClipComponent::ParameterGraph::mouseDown (const MouseEvent& event)
{
    draggingIndex = findClosestKeyFrame (event.x, event.y);

    if (draggingIndex >= 0 && event.mods.isCtrlDown())
    {
        automation.deleteKeyframe (draggingIndex);
        repaint();
        return;
    }

    if (draggingIndex < 0)
    {
        if (automation.getKeyframes().empty() && event.mods.isCtrlDown() == false)
        {
            automation.setValue (mapToValue (event.y));
        }
        else
        {
            automation.addKeyframe (mapToTime (event.x), mapToValue (event.y));
            draggingIndex = findClosestKeyFrame (event.x, event.y);
        }
    }
}

void TimeLine::ClipComponent::ParameterGraph::mouseDrag (const MouseEvent& event)
{
    if (automation.getKeyframes().empty())
    {
        automation.setValue (mapToValue (event.y));
    }
    else if (isPositiveAndBelow (draggingIndex, automation.getKeyframes().size()))
    {
        const auto time = mapToTime (event.x);
        const auto value = mapToValue (event.y);

        automation.setKeyframe (draggingIndex, time, value);
        draggingIndex = findClosestKeyFrame (event.x, jlimit (1, getHeight() - 2, event.y));
    }

    repaint();
}

void TimeLine::ClipComponent::ParameterGraph::mouseUp (const MouseEvent&)
{
    draggingIndex = -1;
}

bool TimeLine::ClipComponent::ParameterGraph::hitTest (int x, int y)
{
    if (draggingIndex >= 0)
        return true;

    auto key = findClosestKeyFrame (x, y);
    if (key >= 0)
        return true;

    return std::abs (mapFromValue (automation.getValueForTime (mapToTime (x))) - y) < 3.0;
}

int TimeLine::ClipComponent::ParameterGraph::findClosestKeyFrame (int x, int y) const
{
    int index = 0;
    for (const auto& it : automation.getKeyframes())
    {
        if (std::abs (mapFromTime (it.first) - x) < 4 && std::abs (mapFromValue (it.second) - y) < 5)
            return index;

        ++index;
    }

    return -1;
}

int TimeLine::ClipComponent::ParameterGraph::mapFromTime (double time) const
{
    return roundToInt (jmap (time, owner.getLeftTime(), owner.getRightTime(), 1.0, getWidth() - 2.0));
}

int TimeLine::ClipComponent::ParameterGraph::mapFromValue (double value) const
{
    return roundToInt (jmap (value, getHeight()-1.0, 1.0));
}

double TimeLine::ClipComponent::ParameterGraph::mapToTime (int x) const
{
    return jmap (double (x), 1.0, getWidth() - 2.0, owner.getLeftTime(), owner.getRightTime());
}

double TimeLine::ClipComponent::ParameterGraph::mapToValue (int y) const
{
    const auto h = getHeight();
    if (h < 2)
        return 0;

    return 1.0 - (y - 1.0) / (h - 2.0);
}

void TimeLine::ClipComponent::ParameterGraph::parameterAutomationChanged (const foleys::ParameterAutomation* a)
{
    if (a == &automation)
        repaint();
}
