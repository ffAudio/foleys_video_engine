/*
  ==============================================================================

    PlayerWindow.h
    Created: 8 Oct 2020 9:42:22pm
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class PlayerWindow  : public juce::TopLevelWindow
{
public:
    PlayerWindow (bool shouldUseOpenGL);

    void resized() override;

    std::unique_ptr<foleys::VideoView> video;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayerWindow)
};
