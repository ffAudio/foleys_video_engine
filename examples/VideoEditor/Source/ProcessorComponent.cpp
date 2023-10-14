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

    ProcessorComponent.cpp
    Created: 25 May 2019 11:18:43am
    Author:  Daniel Walz

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ProcessorComponent.h"
#include "Player.h"

//==============================================================================

namespace IDs
{
    static Identifier collapsed { "collapsed" };
}

AutomationComponent::AutomationComponent (const juce::String& titleToUse,
                                          foleys::AVClip& clipToUse,
                                          foleys::ControllableBase& controllerToUse,
                                          Player& player)
  : title (titleToUse),
    clip (clipToUse),
    controller (controllerToUse)
{
    if (auto* processorController = dynamic_cast<foleys::ProcessorController*>(&controller))
    {
        processorControls = std::make_unique<ProcessorControls>(*processorController);
        addAndMakeVisible (processorControls.get());
        processorControls->addChangeListener (this);
    }

    std::vector<foleys::ParameterAutomation*> automations;
    for (auto& pair : controller.getParameters())
        automations.push_back (pair.second.get());

    std::sort (automations.begin(), automations.end(), [](const auto& a, const auto& b) { return a->getParameterIndex() < b->getParameterIndex(); });

    for (auto* automation : automations)
    {
        auto component = std::make_unique<ParameterComponent>(controller.getTimeReference(), *automation, player);
        addAndMakeVisible (component.get());
        parameterComponents.push_back (std::move (component));
    }

    controller.addListener (this);
    clip.addTimecodeListener (this);
}

AutomationComponent::~AutomationComponent()
{
    clip.removeTimecodeListener (this);
    controller.removeListener (this);

    if (processorControls.get() != nullptr)
        processorControls->removeChangeListener (this);
}

void AutomationComponent::paint (Graphics& g)
{
    g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0);

    g.setColour (Colours::grey);
    g.drawRoundedRectangle (getLocalBounds().toFloat(), 6.0, 1);

    g.setColour (Colours::silver);
    g.setFont (16.0f);

    auto area = getLocalBounds();

    g.drawText (title, area.removeFromTop (24).reduced (33, 3),
                Justification::left, true);
}

void AutomationComponent::resized()
{
    auto area = getLocalBounds().reduced (3);

    if (processorControls.get() != nullptr)
        processorControls->setBounds (area.removeFromTop (24).reduced (3, 0));
    else
        area.removeFromTop (24);

    auto collapsed = (processorControls.get() != nullptr) ? processorControls->isCollapsed() : false;

    for (auto& c : parameterComponents)
    {
        if (! collapsed)
        {
            c->setVisible (true);
            c->setBounds (area.removeFromTop (40));
        }
        else
        {
            c->setVisible (false);
        }
    }
}

void AutomationComponent::mouseDrag (const MouseEvent& event)
{
    if (event.getDistanceFromDragStart() < 5)
        return;

    if (auto* processorController = dynamic_cast<foleys::ProcessorController*>(&controller))
    {
        if (auto* dndContainer = findParentComponentOfClass<DragAndDropContainer>())
        {
            processorController->readPluginStatesIntoValueTree();
            dndContainer->startDragging (processorController->getProcessorState().toXmlString(), this);
        }
    }
}

int AutomationComponent::getHeightForWidth(int width) const
{
    // todo: adapt to width
    ignoreUnused (width);

    auto height = 40 * controller.getNumParameters();

    if (processorControls.get() != nullptr)
    {
        if (processorControls->isCollapsed())
            return 40;
    }

    return height + 40;
}

void AutomationComponent::timecodeChanged (int64_t, double)
{
    auto localTime = controller.getCurrentPTS();

    for (auto& c : parameterComponents)
        c->updateForTime (localTime);
}

void AutomationComponent::parameterAutomationChanged (const foleys::ParameterAutomation*)
{
    auto seconds = controller.getCurrentPTS();
    for (auto& c : parameterComponents)
        c->updateForTime (seconds);
}

const foleys::ProcessorController* AutomationComponent::getProcessorController() const
{
    if (processorControls.get() != nullptr)
        return &processorControls->getProcessorController();

    return nullptr;
}

void AutomationComponent::changeListenerCallback (ChangeBroadcaster*)
{
    sendChangeMessage();
}

//==============================================================================

class ParameterSlider : public AutomationComponent::ParameterComponent::ParameterWidget
{
public:
    ParameterSlider (foleys::ParameterAutomation& parameter, foleys::TimeCodeAware& timeReference)
    {
        const auto numSteps = parameter.getNumSteps();

        NormalisableRange<double> range;
        if (numSteps > 0)
            range.interval = 1.0 / numSteps;

        valueSlider.onDragStart = [&]
        {
            parameter.startAutomationGesture();
            dragging = true;
        };

        valueSlider.onDragEnd = [&]
        {
            dragging = false;
            parameter.finishAutomationGesture();
        };

        valueSlider.onValueChange = [&]
        {
            if (dragging)
                parameter.setValue (timeReference.getCurrentTimeInSeconds(), valueSlider.getValue());
        };

        valueSlider.textFromValueFunction = [&parameter](double value) { return parameter.getText (float(value)); };
        valueSlider.valueFromTextFunction = [&parameter](String text) { return parameter.getValueForText (text); };

        valueSlider.setNormalisableRange (range);

        auto colour = Colour::fromString (parameter.getParameterProperties().getWithDefault ("Colour", "ffa0a0a0").toString());
        valueSlider.setColour (Slider::trackColourId, colour);
    }

    void setValue (double value) override
    {
        if (!dragging)
            valueSlider.setValue (value);
    }

    double getValue() const override
    {
        return valueSlider.getValue();
    }

    juce::Component& getComponent() override
    {
        return valueSlider;
    }

private:
    Slider valueSlider { Slider::LinearHorizontal, Slider::TextBoxRight };
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

class ParameterChoice : public AutomationComponent::ParameterComponent::ParameterWidget
{
public:
    ParameterChoice (foleys::ParameterAutomation& parameter, foleys::TimeCodeAware&)
    {
        choice.addItemList (parameter.getAllValueStrings(), 1);
        choice.onChange = [&]
        {
            const auto numChoices = choice.getNumItems();
            parameter.setValue (choice.getSelectedItemIndex() / (numChoices - 1.0));
        };
    }

    void setValue (double value) override
    {
        auto numChoices = choice.getNumItems();
        if (numChoices > 1)
            choice.setSelectedItemIndex (roundToInt (value * (numChoices - 1.0)));
        else
            choice.setSelectedItemIndex (0);
    }

    double getValue() const override
    {
        auto numChoices = choice.getNumItems();
        if (numChoices > 1)
            return roundToInt (choice.getSelectedItemIndex() / (numChoices - 1.0));

        return 0;
    }

    juce::Component& getComponent() override
    {
        return choice;
    }

private:
    ComboBox choice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterChoice)
};

class ParameterSwitch : public AutomationComponent::ParameterComponent::ParameterWidget
{
public:
    ParameterSwitch (foleys::ParameterAutomation& parameter, foleys::TimeCodeAware& timeReference)
    {
        button.setButtonText (parameter.getText (1.0f));
        button.setClickingTogglesState (true);
        button.onClick = [&]
        {
            parameter.startAutomationGesture();
            parameter.setValue (timeReference.getCurrentTimeInSeconds(), button.getToggleState() ? 1.0 : 0.0);
            parameter.finishAutomationGesture();
        };
    }

    void setValue (double value) override
    {
        button.setToggleState (value > 0.5, dontSendNotification);
    }

    double getValue() const override
    {
        return button.getToggleState() ? 1.0 : 0.0;
    }

    juce::Component& getComponent() override
    {
        return button;
    }

private:
    TextButton button;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSwitch)
};

//==============================================================================

AutomationComponent::ParameterComponent::ParameterComponent (foleys::TimeCodeAware& reference,
                                                            foleys::ParameterAutomation& parameterToControl,
                                                            Player& player)
  : timeReference (reference),
    parameter (parameterToControl)
{
    prev.setConnectedEdges (TextButton::ConnectedOnRight);
    next.setConnectedEdges (TextButton::ConnectedOnRight | TextButton::ConnectedOnLeft);
    add.setConnectedEdges (TextButton::ConnectedOnLeft);

    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    addAndMakeVisible (add);

    auto numSteps = parameter.getNumSteps();

    auto options = parameter.getAllValueStrings();
    if (numSteps == 2)
    {
        widget = std::make_unique<ParameterSwitch>(parameter, timeReference);
    }
    else if (! options.isEmpty())
    {
        widget = std::make_unique<ParameterChoice>(parameter, timeReference);
    }
    else
    {
        widget = std::make_unique<ParameterSlider>(parameter, timeReference);
    }

    widget->setValue (parameter.getValue());
    addAndMakeVisible (widget->getComponent());

    add.onClick = [this]
    {
        parameter.addKeyframe (timeReference.getCurrentTimeInSeconds(), widget->getValue());
    };

    prev.onClick = [&]
    {
        auto p = parameter.getPreviousKeyframeTime (timeReference.getCurrentTimeInSeconds());
        if (auto* descriptor = dynamic_cast<foleys::ClipDescriptor*>(&reference))
            player.setPosition (p + descriptor->getStart() - descriptor->getOffset());
    };

    next.onClick = [&]
    {
        auto n = parameter.getNextKeyframeTime (timeReference.getCurrentTimeInSeconds());
        if (auto* descriptor = dynamic_cast<foleys::ClipDescriptor*>(&reference))
            player.setPosition (n + descriptor->getStart() - descriptor->getOffset());
    };

}

void AutomationComponent::ParameterComponent::paint (Graphics& g)
{
    auto area = getLocalBounds().reduced (3);
    g.setColour (Colours::silver);
    g.drawFittedText (parameter.getName(), area, Justification::topLeft, 1);
}

void AutomationComponent::ParameterComponent::resized()
{
    auto area = getLocalBounds().reduced (3);
    add.setBounds (area.removeFromRight (24).withTop (area.getHeight() - 24));
    next.setBounds (area.removeFromRight (24).withTop (area.getHeight() - 24));
    prev.setBounds (area.removeFromRight (24).withTop (area.getHeight() - 24));
    widget->getComponent().setBounds (area.withTop (20).withTrimmedRight (3));
}

void AutomationComponent::ParameterComponent::updateForTime (double pts)
{
    widget->setValue (parameter.getValueForTime (pts));
}

//==============================================================================

AutomationComponent::ProcessorControls::ProcessorControls (foleys::ProcessorController& controllerToUse)
  : controller (controllerToUse)
{
    editorButton.setConnectedEdges (Button::ConnectedOnRight);
    removeButton.setConnectedEdges (Button::ConnectedOnRight);
    collapseButton.setConnectedEdges (Button::ConnectedOnLeft);

    activeButton.setClickingTogglesState (true);
    activeButton.setToggleState (controller.isActive(), dontSendNotification);
    addAndMakeVisible (activeButton);
    activeButton.onStateChange = [&]
    {
        controller.setActive (activeButton.getToggleState());
    };
    activeButton.setColour (TextButton::buttonOnColourId, Colours::green);

    addChildComponent (editorButton);
    if (auto* processor = controller.getAudioProcessor())
    {
        if (processor->hasEditor())
        {
            removeButton.setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);

            editorButton.setVisible (processor->hasEditor());
            editorButton.onClick = [&]
            {
                if (auto* p = controller.getAudioProcessor())
                    showProcessorEditor (p->createEditor(), p->getName() + " - " + controller.getOwningClipDescriptor().getDescription());

                sendChangeMessage();
            };
        }
    }

    collapseButton.setClickingTogglesState (true);
    collapseButton.setToggleState (isCollapsed(), dontSendNotification);
    addAndMakeVisible (collapseButton);
    collapseButton.onClick = [&]
    {
        controller.getProcessorState().setProperty (IDs::collapsed, collapseButton.getToggleState(), nullptr);
        sendChangeMessage();
    };

    addAndMakeVisible (removeButton);
    removeButton.onClick = [&]
    {
        controller.getOwningClipDescriptor().removeProcessor (&controller);
    };

    controller.getOwningClipDescriptor().addListener (this);
}

AutomationComponent::ProcessorControls::~ProcessorControls()
{
    controller.getOwningClipDescriptor().removeListener (this);
}

void AutomationComponent::ProcessorControls::showProcessorEditor (AudioProcessorEditor* editor, const String& editorTitle)
{
    audioProcessorWindow = std::make_unique<AudioProcessorWindow>(editor, editorTitle);
    audioProcessorWindow->centreAroundComponent (getTopLevelComponent(), audioProcessorWindow->getWidth(), audioProcessorWindow->getHeight());
}

void AutomationComponent::ProcessorControls::resized()
{
    auto bounds = getLocalBounds();

    activeButton.setBounds (bounds.removeFromLeft (getHeight()));

    collapseButton.setBounds (bounds.removeFromRight (getHeight()));
    removeButton.setBounds (bounds.removeFromRight (getHeight()));
    editorButton.setBounds (bounds.removeFromRight (getHeight()));
}

bool AutomationComponent::ProcessorControls::isCollapsed() const
{
    return controller.getProcessorState().getProperty (IDs::collapsed, false);
}

void AutomationComponent::ProcessorControls::processorControllerToBeDeleted (const foleys::ProcessorController* controllerToBeDeleted)
{
    if (controllerToBeDeleted == &controller)
        audioProcessorWindow.reset();
}

foleys::ProcessorController& AutomationComponent::ProcessorControls::getProcessorController()
{
    return controller;
}

//==============================================================================

AutomationComponent::AudioProcessorWindow::AudioProcessorWindow (AudioProcessorEditor* editor, const String& titleToUse)
  : DocumentWindow (titleToUse, Colours::darkgrey, DocumentWindow::closeButton, true)
{
    setAlwaysOnTop (true);
    setWantsKeyboardFocus (false);
    setUsingNativeTitleBar (true);
    setResizable (editor->isResizable(), false);
    setContentOwned (editor, true);
    setVisible (true);
}

void AutomationComponent::AudioProcessorWindow::closeButtonPressed()
{
    setVisible (false);
    setContentOwned (nullptr, false);
}
