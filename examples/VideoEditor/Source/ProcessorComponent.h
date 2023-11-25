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

    ProcessorComponent.h
    Created: 25 May 2019 11:18:43am
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class Player;

//==============================================================================
/*
*/
class AutomationComponent   : public Component,
                              public ChangeBroadcaster,
                              private ChangeListener,
                              private foleys::ControllableBase::Listener,
                              private foleys::TimeCodeAware::Listener
{
public:
    AutomationComponent (const juce::String& title,
                         foleys::AVClip& clip,
                         foleys::ControllableBase& controller,
                         Player& player);
    ~AutomationComponent() override;

    void paint (Graphics&) override;
    void resized() override;

    void mouseDrag (const MouseEvent&) override;

    int getHeightForWidth(int width) const;

    void timecodeChanged (int64_t count, double seconds) override;

    const foleys::ProcessorController* getProcessorController() const;

    void parameterAutomationChanged (const foleys::ParameterAutomation*) override;

    void changeListenerCallback (ChangeBroadcaster* sender) override;

    class ParameterComponent : public juce::Component, private foleys::ProcessorParameter::Listener
    {
    public:
        ParameterComponent (foleys::TimeCodeAware& timeReference, foleys::ParameterAutomation& parameter, Player& player);

        void paint (Graphics&) override;
        void resized() override;

        void valueChanged (foleys::ProcessorParameter&, double) override {}
        void gestureStarted (foleys::ProcessorParameter&) override {}
        void gestureFinished (foleys::ProcessorParameter&) override {}

        void updateForTime (double pts);

        class ParameterWidget
        {
        public:
            ParameterWidget() = default;
            virtual ~ParameterWidget() = default;

            virtual void setValue (double value) = 0;
            virtual double getValue() const = 0;

            virtual juce::Component& getComponent() = 0;

        private:
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterWidget)
        };

    private:
        foleys::TimeCodeAware& timeReference;
        foleys::ParameterAutomation& parameter;

        std::unique_ptr<ParameterWidget> widget;

        TextButton prev { "<" };
        TextButton next { ">" };
        TextButton add  { "+" };
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterComponent)
    };
private:

    String                    title;
    foleys::AVClip&           clip;
    foleys::ControllableBase& controller;

    std::vector<std::unique_ptr<ParameterComponent>> parameterComponents;

    //==============================================================================

    class AudioProcessorWindow  : public DocumentWindow
    {
    public:
        AudioProcessorWindow (AudioProcessorEditor* editor, const String& title);
        void closeButtonPressed() override;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorWindow)
    };

    //==============================================================================

    class ProcessorControls  : public Component,
                               public ChangeBroadcaster,
                               private foleys::ClipDescriptor::Listener
    {
    public:
        ProcessorControls (foleys::ProcessorController& controller);
        ~ProcessorControls() override;

        void showProcessorEditor (AudioProcessorEditor* editor, const String& title);
        void resized() override;

        bool isCollapsed() const;

        void processorControllerAdded() override {}
        void processorControllerToBeDeleted (const foleys::ProcessorController*) override;

        foleys::ProcessorController& getProcessorController();

    private:
        foleys::ProcessorController&    controller;

        TextButton activeButton   { "A" };
        TextButton editorButton   { "E" };
        TextButton collapseButton { "v" };
        TextButton removeButton   { "X" };

        std::unique_ptr<AudioProcessorWindow> audioProcessorWindow;

        JUCE_DECLARE_NON_COPYABLE (ProcessorControls)
    };
    std::unique_ptr<ProcessorControls> processorControls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomationComponent)
};
