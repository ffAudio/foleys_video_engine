#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible (view);

    editClip = std::make_shared<foleys::ComposedClip> (videoEngine);
    view.setClip (editClip);
    transport.setSource (editClip.get());
    transport.addChangeListener (this);

    auto cameras = cameraManager.getCameraInfos();

    for (auto& camera: cameras) DBG (camera.name << " found (" << camera.uid << ")");

    const auto numCameras = cameras.size();
    if (numCameras > 0) addCamera (0, 49.0, -0.25, -0.25);
    if (numCameras > 1) addCamera (1, 49.0,  0.25, -0.25);
    if (numCameras > 2) addCamera (2, 49.0, -0.25,  0.25);
    if (numCameras > 3) addCamera (3, 49.0,  0.25,  0.25);

    setAudioChannels (0, 2);

    transport.start();

    setSize (800, 400);
}

MainComponent::~MainComponent()
{
    transport.removeChangeListener (this);
    transport.setSource (nullptr);

    shutdownAudio();
}

//==============================================================================

void MainComponent::addCamera (int index, float zoom, float posX, float posY)
{
    auto camera = cameraManager.openCamera (index);

    DBG ("Camera UID: " << camera->getCameraUid());

    camera->onCaptureEngineInitialized = [cameraPtr = camera.get()]() 
    {
        auto resolutions = cameraPtr->getAvailableResolutions();
        int  i           = 0;
        for (auto r: resolutions) DBG (juce::String (i++) << ": " << r.toString());

        //cameraPtr->setResolution (6);

        auto success = cameraPtr->start();
        DBG((success ? "Start successful" : "Start didn't succeed"));
    };

    auto  cameraClip = cameraManager.createCameraClip (std::move (camera));

    foleys::ComposedClip::ClipPosition position;
    position.length = 3600.0;
    auto descriptor = editClip->addClip (cameraClip, position);

    descriptor->getVideoParameterController().getParameters()["zoom"]->setRealValue (zoom);
    descriptor->getVideoParameterController().getParameters()["translateX"]->setRealValue (posX);
    descriptor->getVideoParameterController().getParameters()["translateY"]->setRealValue (posY);
}

//==============================================================================

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double newSampleRate)
{
    transport.prepareToPlay (samplesPerBlockExpected, newSampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    transport.getNextAudioBlock (bufferToFill);
}

void MainComponent::releaseResources()
{
    transport.releaseResources();
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    view.setContinuousRepaint (transport.isPlaying() ? 0 : 30);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    view.setBounds (getLocalBounds());
}
