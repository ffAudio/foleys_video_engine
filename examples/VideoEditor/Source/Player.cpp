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

    Player.cpp
    Created: 31 Mar 2019 10:23:27pm
    Author:  Daniel Walz

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Player.h"

//==============================================================================
Player::Player (AudioDeviceManager& deviceManagerToUse,
                foleys::VideoEngine& engine)
  : deviceManager (deviceManagerToUse),
    videoEngine (engine)
{
}

Player::~Player()
{
    shutDown();
}

void Player::start()
{
    stopAudition();

    transportSource.start();
    sendChangeMessage();
}

void Player::stop()
{
    transportSource.stop();
    sendChangeMessage();
}

bool Player::isPlaying()
{
    return transportSource.isPlaying();
}

void Player::setPosition (double pts)
{
    if (isAuditioning())
        stopAudition();

    if (clip)
        clip->setNextReadPosition (int64 (pts * getSampleRate()));

    sendChangeMessage();
}

void Player::nextFrame()
{
    if (clip.get() == nullptr || transportSource.isPlaying())
        return;

    auto samples = clip->getNextReadPosition();
    clip->setNextReadPosition (int64 (samples + clip->getFrameDurationInSeconds() * getSampleRate()));
}

void Player::previousFrame()
{
    if (clip.get() == nullptr || transportSource.isPlaying())
        return;

    auto samples = clip->getNextReadPosition();
    clip->setNextReadPosition (int64 (samples - clip->getFrameDurationInSeconds() * getSampleRate()));
}

double Player::getCurrentTimeInSeconds() const
{
    if (clip)
        return clip->getCurrentTimeInSeconds();

    return {};
}

void Player::setClip (std::shared_ptr<foleys::AVClip> clipToUse, bool needsPrepare)
{
    auto numChannels = 2;
    transportSource.stop();
    transportSource.setSource (nullptr);
    clip = clipToUse;
    if (needsPrepare && clip != nullptr)
    {
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            clip->prepareToPlay (device->getDefaultBufferSize(), device->getCurrentSampleRate());
            numChannels = device->getOutputChannelNames().size();
        }
    }

    transportSource.setSource (clip.get());
    transportSource.meterSource.resize (numChannels, 5);

    sendChangeMessage();
}

void Player::setAuditionFile (const File& file)
{
    auto* reader = videoEngine.getAudioFormatManager().createReaderFor (file);
    if (reader != nullptr)
    {
        auto sampleRate = reader->sampleRate;
        setAuditionSource (std::make_unique<AudioFormatReaderSource>(reader, true), sampleRate);
    }
}

void Player::setAuditionSource (std::unique_ptr<PositionableAudioSource> source, double sampleRate)
{
    transportSource.stop();
    auditionTransport.setSource (nullptr);

    auditionSource = std::move (source);
    if (auditionSource.get() == nullptr)
        return;

    if (auto* device = deviceManager.getCurrentAudioDevice())
        auditionSource->prepareToPlay (device->getDefaultBufferSize(), device->getCurrentSampleRate());

    auditionTransport.setSource (auditionSource.get(), 0, nullptr, sampleRate);
    auditionTransport.start();
}

void Player::stopAudition()
{
    auditionTransport.stop();
}

bool Player::isAuditioning() const
{
    return auditionTransport.isPlaying();
}

void Player::initialise ()
{
    deviceManager.initialise (0, 2, nullptr, true);
    deviceManager.addChangeListener (this);

    mixingSource.addInputSource (&transportSource, false);
    mixingSource.addInputSource (&auditionTransport, false);

    if (auto* device = deviceManager.getCurrentAudioDevice())
        mixingSource.prepareToPlay (device->getDefaultBufferSize(), device->getCurrentSampleRate());

    sourcePlayer.setSource (&mixingSource);
    deviceManager.addAudioCallback (&sourcePlayer);

    transportSource.meterSource.resize (2, 10);
}

void Player::shutDown ()
{
    deviceManager.removeChangeListener (this);
    sourcePlayer.setSource (nullptr);
    deviceManager.removeAudioCallback (&sourcePlayer);
}

double Player::getSampleRate() const
{
    if (deviceManager.getCurrentAudioDevice() != nullptr)
        return deviceManager.getCurrentAudioDevice()->getCurrentSampleRate();

    return 0;
}

void Player::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        if (clip != nullptr)
            clip->prepareToPlay (device->getDefaultBufferSize(), device->getCurrentSampleRate());

        mixingSource.prepareToPlay (device->getDefaultBufferSize(), device->getCurrentSampleRate());
    }
}

foleys::LevelMeterSource& Player::getMeterSource()
{
    return transportSource.meterSource;
}
