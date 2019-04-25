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
 */

namespace foleys
{

MovieClip::MovieClip (VideoEngine& engine)
  : AVClip (engine)
{
}

juce::String MovieClip::getDescription() const
{
    if (movieReader)
        return movieReader->getMediaFile().getFileNameWithoutExtension();

    return "MovieClip";
}

bool MovieClip::openFromFile (const juce::File file)
{
    if (videoEngine == nullptr)
        return false;

    backgroundJob.setSuspended (true);

    auto reader = videoEngine->createReaderFor (file);
    if (reader->isOpenedOk())
    {
        if (reader->hasVideo())
            setThumbnailReader (videoEngine->createReaderFor (file, StreamTypes::video()));
        else
            setThumbnailReader ({});

        setReader (std::move (reader));
        backgroundJob.setSuspended (false);
        return true;
    }

    return false;
}

juce::File MovieClip::getMediaFile() const
{
    if (movieReader)
        return movieReader->getMediaFile();

    return {};
}

void MovieClip::setReader (std::unique_ptr<AVReader> readerToUse)
{
    backgroundJob.setSuspended (true);

    movieReader = std::move (readerToUse);
    audioFifo.setNumChannels (movieReader->numChannels);
    audioFifo.setSampleRate (movieReader->sampleRate);
    audioFifo.setPosition (0);

    auto& settings = videoFifo.getVideoSettings();
    settings.timebase = movieReader->timebase;
    settings.frameSize = movieReader->originalSize;
    videoFifo.clear (0);

    backgroundJob.setSuspended (false);
}

void MovieClip::setThumbnailReader (std::unique_ptr<AVReader> reader)
{
    thumbnailReader = std::move (reader);
}

Size MovieClip::getVideoSize() const
{
    return (movieReader != nullptr) ? movieReader->originalSize : Size();
}

double MovieClip::getLengthInSeconds() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength() / sampleRate;

    return {};
}

double MovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

Timecode MovieClip::getCurrentTimecode() const
{
    return getFrameTimecodeForTime (getCurrentTimeInSeconds());
}

Timecode MovieClip::getFrameTimecodeForTime (double time) const
{
    return videoFifo.getFrameTimecodeForTime (time);
}

juce::Image MovieClip::getFrame (double pts) const
{
    return videoFifo.getVideoFrame (pts);
}

bool MovieClip::isFrameAvailable (double pts) const
{
    return videoFifo.isFrameAvailable (pts);
}

juce::Image MovieClip::getStillImage (double seconds, Size size)
{
    if (thumbnailReader && thumbnailReader->isOpenedOk())
        return thumbnailReader->getStillImage (seconds, size);

    return {};
}

juce::Image MovieClip::getCurrentFrame() const
{
    auto pts = sampleRate > 0 ? nextReadPosition / sampleRate : 0.0;
    if (movieReader && movieReader->sampleRate > 0)
        pts = audioFifo.getReadPosition() / movieReader->sampleRate;

    return videoFifo.getVideoFrame (pts);
}

void MovieClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    if (movieReader && movieReader->sampleRate > 0)
        movieReader->setOutputSampleRate (sampleRate);
}

void MovieClip::releaseResources()
{
    sampleRate = 0;
}

void MovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    if (movieReader && movieReader->isOpenedOk() && movieReader->hasAudio())
    {
        audioFifo.pullSamples (info);
    }
    else
    {
        info.clearActiveBufferRegion();
    }
    nextReadPosition += info.numSamples;

    triggerAsyncUpdate();
}

bool MovieClip::hasVideo() const
{
    return movieReader ? movieReader->hasVideo() : false;
}

bool MovieClip::hasAudio() const
{
    return movieReader ? movieReader->hasAudio() : false;
}

bool MovieClip::hasSubtitle() const
{
    return movieReader ? movieReader->hasSubtitle() : false;
}

std::shared_ptr<AVClip> MovieClip::createCopy()
{
    if (videoEngine == nullptr)
        return {};

    return videoEngine->createClipFromFile (getMediaFile());
}

double MovieClip::getSampleRate() const
{
    return sampleRate;
}

void MovieClip::handleAsyncUpdate()
{
    if (sampleRate > 0 && hasVideo())
    {
        auto currentTimecode = videoFifo.getFrameTimecodeForTime (nextReadPosition / sampleRate);
        if (currentTimecode != lastShownFrame)
        {
            sendTimecode (currentTimecode, juce::sendNotificationAsync);
            lastShownFrame = currentTimecode;
        }

        videoFifo.clearFramesOlderThan (lastShownFrame);
    }
}

void MovieClip::setNextReadPosition (juce::int64 samples)
{
    backgroundJob.setSuspended (true);

    nextReadPosition = samples;
    audioFifo.setPosition (samples);
    if (movieReader && sampleRate > 0)
    {
        auto time = samples / sampleRate;
        if (sampleRate == movieReader->sampleRate)
        {
            movieReader->setPosition (samples);
        }
        else
        {
            movieReader->setPosition (time * movieReader->sampleRate);
        }
        videoFifo.clear (time * videoFifo.getVideoSettings().timebase);
    }
    else
    {
        videoFifo.clear (0);
    }

    backgroundJob.setSuspended (false);
    triggerAsyncUpdate();
}

juce::int64 MovieClip::getNextReadPosition() const
{
    return nextReadPosition;
}

juce::int64 MovieClip::getTotalLength() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength();

    return 0;
}

bool MovieClip::isLooping() const
{
    return loop;
}

void MovieClip::setLooping (bool shouldLoop)
{
    loop = shouldLoop;
}

MovieClip::BackgroundReaderJob::BackgroundReaderJob (MovieClip& ownerToUse)
    : owner (ownerToUse)
{
}

int MovieClip::BackgroundReaderJob::useTimeSlice()
{
    if (!suspended && owner.movieReader.get() != nullptr && owner.audioFifo.getFreeSpace() > 2048)
    {
        juce::ScopedValueSetter<bool> guard (inDecodeBlock, true);
        owner.movieReader->readNewData (owner.videoFifo, owner.audioFifo);
        return 0;
    }

    return 10;
}

void MovieClip::BackgroundReaderJob::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inDecodeBlock)
        juce::Thread::sleep (5);
}

juce::TimeSliceClient* MovieClip::getBackgroundJob()
{
    return &backgroundJob;
}

} // foleys
