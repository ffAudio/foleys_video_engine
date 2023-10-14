/*
 ==============================================================================
  Copyright (c) 2019-2020, Foleys Finest Audio - Daniel Walz
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

    Demo to play back a movie using ffmpeg and JUCE

 ==============================================================================
 */


#include "../JuceLibraryCode/JuceHeader.h"
#include "OSDComponent.h"

//==============================================================================
/*
 The display plus drop target to allow movies dragged into
 */
#if FOLEYS_USE_OPENGL
using ViewType = foleys::OpenGLView;
#else
using ViewType = foleys::SoftwareView;
#endif

class VideoComponentWithDropper
  : public ViewType
  , public juce::FileDragAndDropTarget
{
 public:
    VideoComponentWithDropper()
    {
        setInterceptsMouseClicks (true, true);
        setWantsKeyboardFocus (false);

        setContinuousRepaint (30);
    }

    bool isInterestedInFileDrag (const juce::StringArray&) override
    {
        return true;
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        if (onFileDropped) onFileDropped (files[0]);

        juce::Process::makeForegroundProcess();
    }

    std::function<void (juce::File)> onFileDropped;

 private:
    JUCE_DECLARE_NON_COPYABLE (VideoComponentWithDropper)
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent
  : public juce::AudioAppComponent
  , public foleys::TimeCodeAware::Listener
{
 public:
    //==============================================================================
    MainContentComponent()
    {
        setWantsKeyboardFocus (true);

#if JUCE_WINDOWS
        videoEngine.getFormatManager().registerFormat (std::make_unique<foleys::MediaFoundationFormat>());
#endif

#if FOLEYS_USE_FFMPEG
        videoEngine.getFormatManager().registerFormat (std::make_unique<foleys::FFmpegFormat>());
#endif
        addAndMakeVisible (filmStrip);

        addAndMakeVisible (videoComponent);
        videoComponent.addAndMakeVisible (osdComponent);

        // specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);

#ifdef DEBUG
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            DBG ("Current Samplerate: " + juce::String (device->getCurrentSampleRate()));
            DBG ("Current Buffersize: " + juce::String (device->getCurrentBufferSizeSamples()));
            DBG ("Current Bitdepth:   " + juce::String (device->getCurrentBitDepth()));
        }
#endif /* DEBUG */

#ifdef USE_FF_AUDIO_METERS
        meter = std::make_unique<foleys::LevelMeter>();
        meter->getLookAndFeel()->setMeterColour (foleys::LevelMeterLookAndFeel::lmBackgroundColour, juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.6f));
        meter->setMeterSource (&meterSource);
        addAndMakeVisible (meter);
#endif

        osdComponent.open.onClick = [this] { openFile(); };

        osdComponent.stop.onClick = [this] {
            transportSource.stop();
            if (clip) clip->setNextReadPosition (0);
        };

        osdComponent.pause.onClick = [this] { transportSource.stop(); };

        osdComponent.play.onClick = [this] {
            if (clip.get() == nullptr) return;

            if (ffwdSpeed != 2)
            {
                auto lastPos = clip->getNextReadPosition();
                ffwdSpeed    = 2;
                auto factor  = 0.5 + (ffwdSpeed / 4.0);
                transportSource.setSource (clip.get(), 0, nullptr, factor, 2);
                clip->setNextReadPosition (lastPos);
            }
            transportSource.start();
        };

        osdComponent.ffwd.onClick = [this] {
            if (clip.get() == nullptr) return;

            auto lastPos = clip->getNextReadPosition();
            ffwdSpeed    = (ffwdSpeed + 1) % 7;
            auto factor  = 0.5 + (ffwdSpeed / 4.0);
            transportSource.setSource (clip.get(), 0, nullptr, factor, 2);
            clip->setNextReadPosition (lastPos);
            transportSource.start();
        };

        osdComponent.seekBar.onValueChange = [this] {
            if (clip.get()) clip->setNextReadPosition (juce::int64 (osdComponent.seekBar.getValue() * sampleRate));
        };

#if FOLEYS_HAS_ADDONS
        osdComponent.camera.onClick = [this] { openCamera(); };
#endif

        videoComponent.onFileDropped = [this] (juce::File file) { openFile (file); };

        setSize (800, 600);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        // This function will be called when the audio device is started, or when
        // its settings (i.e. sample rate, block size, etc) are changed.
        sampleRate = newSampleRate;
        blockSize  = samplesPerBlockExpected;

        if (clip.get() != nullptr) clip->prepareToPlay (blockSize, sampleRate);

        transportSource.prepareToPlay (blockSize, sampleRate);

        readBuffer.setSize (2, blockSize);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto numInputChannels = 2;  // movieClip->getVideoChannels();

        juce::AudioSourceChannelInfo info (&readBuffer, bufferToFill.startSample, bufferToFill.numSamples);
        // the AudioTransportSource takes care of start, stop and resample
        transportSource.getNextAudioBlock (info);

#ifdef USE_FF_AUDIO_METERS
        meterSource.measureBlock (readBuffer);
#endif

        if (numInputChannels > 0)
        {
            for (int i = 0; i < bufferToFill.buffer->getNumChannels(); ++i)
            {

                bufferToFill.buffer->copyFrom (i, bufferToFill.startSample, readBuffer.getReadPointer (i % numInputChannels), bufferToFill.numSamples);
                if (bufferToFill.buffer->getNumChannels() == 2 && readBuffer.getNumChannels() > 2)
                {
                    // add center to left and right
                    bufferToFill.buffer->addFrom (i, bufferToFill.startSample, readBuffer.getReadPointer (2), bufferToFill.numSamples, 0.7f);
                }
            }
        }
        else
        {
            bufferToFill.clearActiveBufferRegion();
        }
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
        if (clip.get() != nullptr) clip->releaseResources();
    }

    void timecodeChanged (int64_t, double seconds) override
    {
        osdComponent.seekBar.setValue (seconds, juce::dontSendNotification);
    }

    void openFile (juce::File name)
    {
        if (clip.get() != nullptr) clip->removeTimecodeListener (this);

        osdComponent.setClip ({});
        auto newClip = videoEngine.createClipFromFile (juce::URL (name));

        if (newClip.get() == nullptr) return;

        newClip->prepareToPlay (blockSize, sampleRate);

        clip = newClip;

        osdComponent.setClip (clip);
        videoComponent.setClip (clip);
        clip->addTimecodeListener (this);
        osdComponent.seekBar.setRange (0.0, clip->getLengthInSeconds());
        transportSource.setSource (clip.get(), 0, nullptr);

        filmStrip.setClip (clip);
        filmStrip.setStartAndEnd (0.0, clip->getLengthInSeconds());
    }

    void openFile()
    {
        transportSource.stop();
        transportSource.setSource (nullptr);

        juce::FileChooser chooser ("Open Video File");
        if (chooser.browseForFileToOpen())
        {
            auto video = chooser.getResult();
            openFile (chooser.getResult());
        }
    }

#if FOLEYS_HAS_ADDONS
    void openCamera()
    {
        osdComponent.setClip ({});
        transportSource.stop();
        transportSource.setSource (nullptr);

        auto camera                        = cameraManager.openCamera (0);
        camera->onCaptureEngineInitialized = [c = camera.get()]() { c->start(); };

        auto newClip = cameraManager.createCameraClip (std::move (camera));
        videoEngine.manageLifeTime (newClip);
        newClip->prepareToPlay (blockSize, sampleRate);
        clip = newClip;
        videoComponent.setClip (clip);
        clip->addTimecodeListener (this);
        transportSource.setSource (clip.get(), 0, nullptr);
    }
#endif

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);
    }

    void resized() override
    {
        auto bounds      = getBounds();
        filmStrip.setBounds (bounds.removeFromBottom (100));

        videoComponent.setBounds (bounds);
        osdComponent.setBounds (bounds);

#ifdef USE_FF_AUDIO_METERS
        const int w = 30 + 20 * videoReader->getVideoChannels();
        meter->setBounds (getWidth() - w, getHeight() - 240, w, 200);
#endif
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey)
        {
            if (transportSource.isPlaying())
            {
                transportSource.stop();
                return true;
            }
            transportSource.start();
            return true;
        }
        return false;
    }

 private:
    //==============================================================================

    foleys::VideoEngine videoEngine;

#if FOLEYS_CAMERA_SUPPORT
    foleys::CameraManager cameraManager { videoEngine };
#endif

    std::shared_ptr<foleys::AVClip> clip;

    juce::AudioTransportSource transportSource;
    double                     sampleRate = 0.0;
    int                        blockSize  = 0;
    int                        ffwdSpeed  = 2;

    VideoComponentWithDropper videoComponent;
    OSDComponent              osdComponent;
    foleys::FilmStrip         filmStrip;

#ifdef USE_FF_AUDIO_METERS
    std::unique_ptr<LevelMeter> meter;
    LevelMeterSource            meterSource;
#endif

    juce::AudioBuffer<float> readBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
juce::Component* createMainContentComponent()
{
    return new MainContentComponent();
}
