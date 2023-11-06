#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent
  : public juce::AudioAppComponent
  , private ::juce::ChangeListener
{
 public:
    //==============================================================================

    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

 private:
    //==============================================================================
    void addCamera (int index, float zoom, float posX, float posY);

    juce::AudioTransportSource transport;

    foleys::VideoEngine   videoEngine;
    foleys::CameraManager cameraManager { videoEngine };

    std::shared_ptr<foleys::ComposedClip> editClip;

    foleys::SoftwareView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
