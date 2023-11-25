/*
 ==============================================================================
 Copyright (c) 2019, Foleys Finest Audio - Daniel Walz
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.

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

 Overlay component

 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class OSDComponent    : public juce::Component
{
public:
    OSDComponent()
    {
        setOpaque (false);
        setInterceptsMouseClicks (false, true);
        setWantsKeyboardFocus (false);

        open.setWantsKeyboardFocus (false);
        addAndMakeVisible (open);
        flexBox.items.add (juce::FlexItem (open).withFlex (1.0, 1.0, 0.5).withHeight (20.0));

#if FOLEYS_CAMERA_SUPPORT
        camera.setWantsKeyboardFocus (false);
        addAndMakeVisible (camera);
        flexBox.items.add (juce::FlexItem (camera).withFlex (1.0, 1.0, 0.5).withHeight (20.0));
#endif

        addAndMakeVisible (seekBar);
        seekBar.setWantsKeyboardFocus (false);
        flexBox.items.add (juce::FlexItem (seekBar).withFlex (6.0, 1.0, 0.5).withHeight (20.0));

        stop.setWantsKeyboardFocus (false);
        addAndMakeVisible (stop);
        flexBox.items.add (juce::FlexItem (stop).withFlex (1.0, 1.0, 0.5).withHeight (20.0));

        pause.setWantsKeyboardFocus (false);
        addAndMakeVisible (pause);
        flexBox.items.add (juce::FlexItem (pause).withFlex (1.0, 1.0, 0.5).withHeight (20.0));

        play.setWantsKeyboardFocus (false);
        addAndMakeVisible (play);
        flexBox.items.add (juce::FlexItem (play).withFlex (1.0, 1.0, 0.5).withHeight (20.0));

        ffwd.setWantsKeyboardFocus (false);
        addAndMakeVisible (ffwd);
        flexBox.items.add (juce::FlexItem (ffwd).withFlex (1.0, 1.0, 0.5).withHeight (20.0));

#if FOLEYS_CAMERA_SUPPORT
        open.setConnectedEdges   (juce::TextButton::ConnectedOnRight);
        camera.setConnectedEdges (juce::TextButton::ConnectedOnLeft);
#endif

        stop.setConnectedEdges  (juce::TextButton::ConnectedOnRight);
        pause.setConnectedEdges (juce::TextButton::ConnectedOnRight | juce::TextButton::ConnectedOnLeft);
        play.setConnectedEdges  (juce::TextButton::ConnectedOnRight | juce::TextButton::ConnectedOnLeft);
        ffwd.setConnectedEdges  (juce::TextButton::ConnectedOnLeft);
    }

    void paint (juce::Graphics& g) override
    {
        if (clip && clip->getLengthInSeconds() > 0)
        {
            g.setColour (juce::Colours::white);
            g.setFont (24);
            auto size = clip->getVideoSize();
            auto dim = juce::String (size.width) + " x " + juce::String (size.height);
            g.drawFittedText (dim, getLocalBounds(), juce::Justification::topLeft, 1);
            g.drawFittedText (foleys::timecodeToString (clip->getCurrentTimeInSeconds()),
                              getLocalBounds(), juce::Justification::topRight, 1);
        }
    }

    void resized() override
    {
        auto bounds = getBounds().withTop (getHeight() - 50).reduced (10);
        flexBox.performLayout (bounds);
    }

    void setClip (std::shared_ptr<foleys::AVClip> newClip)
    {
        clip = newClip;
    }

    class MouseIdle : public juce::MouseListener, public juce::Timer
    {
    public:
        MouseIdle (Component& c) :
        component (c),
        lastMovement (juce::Time::getMillisecondCounter())
        {
            juce::Desktop::getInstance().addGlobalMouseListener (this);
            startTimerHz (20);
        }

        void timerCallback () override
        {
            const auto relTime = juce::Time::getMillisecondCounter() - lastMovement;
            if (relTime < 2000)
            {
                component.setVisible (true);
                component.setAlpha (1.0);
                if (auto* parent = component.getParentComponent())
                    parent->setMouseCursor (juce::MouseCursor::StandardCursorType::NormalCursor);
            }
            else if (relTime < 2300)
            {
                component.setAlpha (1.0f - std::max (0.0f, (relTime - 2000.0f) / 300.0f));
            }
            else
            {
                component.setVisible (false);
                if (auto* parent = component.getParentComponent())
                {
                    parent->setMouseCursor (juce::MouseCursor::StandardCursorType::NoCursor);
                    juce::Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
                }
            }
        }

        void mouseMove (const juce::MouseEvent &event) override
        {
            if (event.position.getDistanceFrom (lastPosition) > 3.0) {
                lastMovement = juce::Time::getMillisecondCounter();
                lastPosition = event.position;
            }
        }
    private:
        juce::Component&   component;
        juce::int64        lastMovement;
        juce::Point<float> lastPosition;
    };

    juce::Slider            seekBar   { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::TextButton        camera    { TRANS ("Camera") };
    juce::TextButton        open      { TRANS ("Open") };
    juce::TextButton        play      { TRANS ("Play") };
    juce::TextButton        pause     { TRANS ("Pause") };
    juce::TextButton        stop      { TRANS ("Stop") };
    juce::TextButton        ffwd      { TRANS ("FWD") };

private:
    MouseIdle               idle      { *this };
    juce::FlexBox           flexBox;

    std::shared_ptr<foleys::AVClip> clip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSDComponent)
};
