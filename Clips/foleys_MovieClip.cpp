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
    addDefaultAudioParameters (*this);
    addDefaultVideoParameters (*this);
}

juce::String MovieClip::getDescription() const
{
    if (movieReader)
        return movieReader->getMediaFile().getFileNameWithoutExtension();

    return "MovieClip";
}

bool MovieClip::openFromFile (const juce::File file)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return false;

    const auto wasSuspended = backgroundJob.isSuspended();
    backgroundJob.setSuspended (true);

    auto reader = engine->createReaderFor (file);
    if (reader->isOpenedOk())
    {
        if (reader->hasVideo())
            setThumbnailReader (engine->createReaderFor (file, StreamTypes::video()));
        else
            setThumbnailReader ({});

        setReader (std::move (reader));
        backgroundJob.setSuspended (wasSuspended);
        return true;
    }

    return false;
}

juce::URL MovieClip::getMediaFile() const
{
    if (movieReader)
        return juce::URL (movieReader->getMediaFile());

    return {};
}

void MovieClip::setReader (std::unique_ptr<AVReader> readerToUse)
{
    backgroundJob.setSuspended (true);

    movieReader = std::move (readerToUse);
    audioFifo.setNumChannels (movieReader->numChannels);
    audioFifo.setSampleRate (sampleRate);
    audioFifo.setPosition (0);

    if (sampleRate > 0)
        movieReader->setOutputSampleRate (sampleRate);

    auto settings = movieReader->getVideoSettings (0);
    videoFifo.setVideoSettings (settings);
    videoFifo.clear();

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
        return movieReader->getTotalLength() / movieReader->sampleRate;

    return {};
}

double MovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

VideoFrame& MovieClip::getFrame (double pts)
{
    return videoFifo.getFrameSeconds (pts);
}

#if FOLEYS_USE_OPENGL
void MovieClip::render (double pts)
{
    auto& frame = getFrame (pts);
    if (frame.image.isNull())
        return;

    if (! frame.upToDate)
    {
        frame.texture.loadImage (frame.image);
        frame.upToDate = true;
    }

    glEnable(GL_TEXTURE_2D);
    frame.texture.bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    static const GLfloat g_vertex_buffer_data[] = {
       -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
       -1.0f,  1.0f, 0.0f,
    };
    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers (1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData (GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
       0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
       4,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       nullptr             // array buffer offset
    );
    
    // Draw the rectangle !
    glDrawArrays (GL_TRIANGLES, 0, 4); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray (0);


    // Draw a textured quad - FIXME: need a replacement for current OpenGL
//    glBegin (GL_QUADS);
//    glTexCoord2f (0, 0); glVertex3f (0, 0, 0);
//    glTexCoord2f (0, 1); glVertex3f (0, 100, 0);
//    glTexCoord2f (1, 1); glVertex3f (100, 100, 0);
//    glTexCoord2f (1, 0); glVertex3f (100, 0, 0);
//    glEnd();

    frame.texture.unbind();
}
#endif

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

void MovieClip::prepareToPlay (int, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    audioFifo.setNumSamples (juce::roundToInt (sampleRate));
    audioFifo.setSampleRate (sampleRate);

    if (movieReader)
        movieReader->setOutputSampleRate (sampleRate);

    backgroundJob.setSuspended (false);
}

void MovieClip::releaseResources()
{
    sampleRate = 0;
}

bool MovieClip::waitForSamplesReady (int samples, int timeout)
{
    if (movieReader && movieReader->isOpenedOk() && movieReader->hasAudio())
    {
        const auto start = juce::Time::getMillisecondCounter();

        while (audioFifo.getAvailableSamples() < samples && int (juce::Time::getMillisecondCounter() - start) < timeout)
            juce::Thread::sleep (5);

        return audioFifo.getAvailableSamples() >= samples;
    }
    else
    {
        return true;
    }
}

bool MovieClip::waitForFrameReady (double pts, int timeout)
{
    const auto start = juce::Time::getMillisecondCounter();

    while (videoFifo.isFrameAvailable (pts) == false && int (juce::Time::getMillisecondCounter() - start) < timeout)
        juce::Thread::sleep (3);

    return videoFifo.isFrameAvailable (pts);
}

void MovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    const auto gain = float (juce::Decibels::decibelsToGain (getAudioParameters().at(IDs::gain)->getRealValue()));

    if (movieReader && movieReader->isOpenedOk() && movieReader->hasAudio())
    {
        audioFifo.pullSamples (info);
        info.buffer->applyGainRamp (info.startSample, info.numSamples, lastGain, gain);
    }
    else
    {
        info.clearActiveBufferRegion();
    }
    nextReadPosition += info.numSamples;
    lastGain = gain;

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

double MovieClip::getFrameDurationInSeconds() const
{
    if (movieReader.get() != nullptr)
    {
        const auto& settings = movieReader->getVideoSettings (0);
        return double (settings.defaultDuration) / double (settings.timebase);
    }

    return {};
}

std::shared_ptr<AVClip> MovieClip::createCopy (StreamTypes types)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    return engine->createClipFromFile (getMediaFile(), types);
}

double MovieClip::getSampleRate() const
{
    return sampleRate;
}

void MovieClip::handleAsyncUpdate()
{
    if (sampleRate > 0 && hasVideo())
    {
        auto seconds = nextReadPosition / sampleRate;
        const auto& frame = videoFifo.getFrameSeconds (seconds);
        if (frame.timecode != lastShownFrame)
        {
            sendTimecode (frame.timecode, seconds, juce::sendNotificationAsync);
            lastShownFrame = frame.timecode;
        }
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
            movieReader->setPosition (juce::int64 (time * movieReader->sampleRate));
        }
        videoFifo.clear();
    }
    else
    {
        videoFifo.clear();
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
    if (suspended == false &&
        owner.sampleRate > 0 &&
        owner.movieReader.get() != nullptr &&
        owner.audioFifo.getFreeSpace() > 2048)
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

bool MovieClip::BackgroundReaderJob::isSuspended() const
{
    return suspended;
}

juce::TimeSliceClient* MovieClip::getBackgroundJob()
{
    return &backgroundJob;
}

} // foleys
