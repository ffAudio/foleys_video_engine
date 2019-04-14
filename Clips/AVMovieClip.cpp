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

AVMovieClip::AVMovieClip (VideoEngine& engine)
  : AVClip (engine)
{
}

juce::String AVMovieClip::getDescription() const
{
    if (movieReader)
        return movieReader->getMediaFile().getFileNameWithoutExtension();

    return "MovieClip";
}

bool AVMovieClip::openFromFile (const juce::File file)
{
    backgroundJob.setSuspended (true);

    auto reader = AVFormatManager::createReaderFor (file);
    if (reader->isOpenedOk())
    {
        if (reader->hasVideo())
            setThumbnailReader (AVFormatManager::createReaderFor (file, StreamTypes::video()));
        else
            setThumbnailReader ({});

        setReader (std::move (reader));
        backgroundJob.setSuspended (false);
        return true;
    }

    return false;
}

void AVMovieClip::setReader (std::unique_ptr<AVReader> readerToUse)
{
    backgroundJob.setSuspended (true);

    movieReader = std::move (readerToUse);
    audioFifo.setNumChannels (movieReader->numChannels);
    audioFifo.setSampleRate (movieReader->sampleRate);
    audioFifo.setPosition (0);

    videoFifo.setTimebase (movieReader->timebase);
    videoFifo.setSize (movieReader->originalSize);
    videoFifo.clear();

    backgroundJob.setSuspended (false);
}

void AVMovieClip::setThumbnailReader (std::unique_ptr<AVReader> reader)
{
    thumbnailReader = std::move (reader);
}

Size AVMovieClip::getVideoSize() const
{
    return (movieReader != nullptr) ? movieReader->originalSize : Size();
}

double AVMovieClip::getLengthInSeconds() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength() / sampleRate;

    return {};
}

double AVMovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

Timecode AVMovieClip::getCurrentTimecode() const
{
    return getFrameTimecodeForTime (getCurrentTimeInSeconds());
}

Timecode AVMovieClip::getFrameTimecodeForTime (double time) const
{
    return videoFifo.getFrameTimecodeForTime (time);
}

juce::Image AVMovieClip::getFrame (double pts) const
{
    return videoFifo.getVideoFrame (pts);
}

juce::Image AVMovieClip::getStillImage (double seconds, Size size)
{
    if (thumbnailReader)
        return thumbnailReader->getStillImage (seconds, size);

    return {};
}

juce::Image AVMovieClip::getCurrentFrame() const
{
    auto pts = sampleRate > 0 ? nextReadPosition / sampleRate : 0.0;
    if (movieReader && movieReader->sampleRate > 0)
        pts = audioFifo.getReadPosition() / movieReader->sampleRate;

    return videoFifo.getVideoFrame (pts);
}

void AVMovieClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    if (movieReader && movieReader->sampleRate > 0)
        movieReader->setOutputSampleRate (sampleRate);
}

void AVMovieClip::releaseResources()
{
    sampleRate = 0;
}

void AVMovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
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

bool AVMovieClip::hasVideo() const
{
    return movieReader ? movieReader->hasVideo() : false;
}

bool AVMovieClip::hasAudio() const
{
    return movieReader ? movieReader->hasAudio() : false;
}

bool AVMovieClip::hasSubtitle() const
{
    return movieReader ? movieReader->hasSubtitle() : false;
}

double AVMovieClip::getSampleRate() const
{
    return sampleRate;
}

void AVMovieClip::handleAsyncUpdate()
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

void AVMovieClip::setNextReadPosition (juce::int64 samples)
{
    backgroundJob.setSuspended (true);

    nextReadPosition = samples;
    audioFifo.setPosition (samples);
    if (movieReader && sampleRate > 0)
    {
        if (sampleRate == movieReader->sampleRate)
        {
            movieReader->setPosition (samples);
        }
        else
        {
            auto time = samples / sampleRate;
            movieReader->setPosition (time * movieReader->sampleRate);
        }
    }

    videoFifo.clear();

    backgroundJob.setSuspended (false);
    triggerAsyncUpdate();
}

juce::int64 AVMovieClip::getNextReadPosition() const
{
    return nextReadPosition;
}

juce::int64 AVMovieClip::getTotalLength() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength();

    return 0;
}

bool AVMovieClip::isLooping() const
{
    return loop;
}

void AVMovieClip::setLooping (bool shouldLoop)
{
    loop = shouldLoop;
}

AVMovieClip::BackgroundReaderJob::BackgroundReaderJob (AVMovieClip& ownerToUse)
    : owner (ownerToUse)
{
}

int AVMovieClip::BackgroundReaderJob::useTimeSlice()
{
    if (!suspended && owner.movieReader.get() != nullptr && owner.audioFifo.getFreeSpace() > 2048)
    {
        juce::ScopedValueSetter<bool> guard (inDecodeBlock, true);
        owner.movieReader->readNewData (owner.videoFifo, owner.audioFifo);
        return 0;
    }

    return 10;
}

void AVMovieClip::BackgroundReaderJob::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inDecodeBlock)
        juce::Thread::sleep (5);
}

juce::TimeSliceClient* AVMovieClip::getBackgroundJob()
{
    return &backgroundJob;
}

} // foleys
