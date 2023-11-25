/*
  ==============================================================================

    PlayerWindow.cpp
    Created: 8 Oct 2020 9:42:22pm
    Author:  Daniel Walz

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PlayerWindow.h"

//==============================================================================
PlayerWindow::PlayerWindow (bool shouldUseOpenGL) : juce::TopLevelWindow ("Output", true)
{
#if FOLEYS_USE_OPENGL
    if (shouldUseOpenGL)
        video = std::make_unique<foleys::OpenGLView>();
    else
        video = std::make_unique<foleys::SoftwareView>();
#else
    juce::ignoreUnused (shouldUseOpenGL);
    video = std::make_unique<foleys::SoftwareView>();
#endif

    setUsingNativeTitleBar (false);

    if (auto* v = dynamic_cast<juce::Component*>(video.get()))
        addAndMakeVisible (v);
}


void PlayerWindow::resized()
{
    if (auto* v = dynamic_cast<juce::Component*>(video.get()))
        v->setBounds (getLocalBounds());
}
