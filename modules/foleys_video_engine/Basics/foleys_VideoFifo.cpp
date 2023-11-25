/*
 ==============================================================================

 Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
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

VideoFifo::VideoFifo (int size)
{
    for (int i=0; i < size; ++i)
        frames.emplace_back (std::make_unique<VideoFrame>());
}

VideoFrame& VideoFifo::getWritingFrame()
{
    return *frames [size_t (writePosition.load())];
}

void VideoFifo::finishWriting()
{
    auto pos = size_t (writePosition.load());

    if (pos + 1 >= frames.size())
        writePosition.store (0);
    else
        ++writePosition;
}

VideoFrame& VideoFifo::getFrame (int64_t timecode)
{
    auto pos = readPosition.load();
    auto nextPos = findFramePosition (timecode, pos);
    if (nextPos >= 0)
    {
        readPosition.store (nextPos);
        return *frames [size_t (nextPos)];
    }

#if FOLEYS_DEBUG_LOGGING
    FOLEYS_LOG ("miss starting from: " << pos << " timecode: " << timecode);
    dumpTimeCodes();
#endif

    readPosition.store (previousIndex (writePosition.load()));

    return *frames [size_t (pos)];
}

VideoFrame& VideoFifo::getFrameSeconds (double pts)
{
    auto timecode = convertTimecode (pts, settings);
    return getFrame (timecode);
}

VideoFrame& VideoFifo::getLatestFrame()
{
    auto pos = previousIndex (writePosition.load());
    readPosition.store (pos);
    return *frames [size_t (pos)];
}

bool VideoFifo::setTimeCodeSeconds (double pts)
{
    auto timecode = convertTimecode (pts, settings);

    auto pos = readPosition.load();
    auto nextPos = findFramePosition (timecode, pos);
    if (nextPos >= 0)
    {
        readPosition.store (nextPos);
        return true;
    }

    readPosition.store (previousIndex (writePosition.load()));
    return false;
}

int VideoFifo::getNumAvailableFrames() const
{
    auto read = readPosition.load();
    auto write = writePosition.load();

    return (write >= read) ? write - read : int (frames.size()) + (write - read);
}

int VideoFifo::getFreeSpace() const
{
    return int (frames.size()) - getNumAvailableFrames();
}

bool VideoFifo::isFrameAvailable (double pts) const
{
    auto timecode = convertTimecode (pts, settings);
    auto pos = findFramePosition (timecode, readPosition.load());
    return pos >= 0;
}

int VideoFifo::findFramePosition (int64_t timecode, int start) const
{
    // direct hit
    if (juce::isPositiveAndBelow (timecode - frames [size_t (start)]->timecode, settings.defaultDuration))
        return start;

    size_t count = 0;

    // forward seek
//    while (timecode >= frames [size_t (start)]->timecode + settings.defaultDuration)
    while (true)
    {
        FOLEYS_LOG ("Seek forward " << count);
        start = nextIndex (start);
        if (frames [size_t (start)]->timecode < 0)
            return -1;

        if (juce::isPositiveAndBelow (timecode - frames [size_t (start)]->timecode, settings.defaultDuration))
            return start;

        if (++count >= frames.size())
            return -1;
    }

    // backward seek
    while (timecode >= frames [size_t (start)]->timecode + settings.defaultDuration)
    {
        FOLEYS_LOG ("Seek backwards " << count);
        start = previousIndex (start);
        if (frames [size_t (start)]->timecode < 0)
            return -1;

        if (juce::isPositiveAndBelow (timecode - frames [size_t (start)]->timecode, settings.defaultDuration))
            return start;

        if (++count >= frames.size())
            return -1;
    }

    return -1;
}

void VideoFifo::setVideoSettings (const VideoStreamSettings& s)
{
    settings = s;
    FOLEYS_LOG ("FIFO VideoSettings: " << settings.frameSize.toString() << " timebase " << settings.timebase << ", duration " << settings.defaultDuration);
}

double VideoFifo::getFrameDurationInSeconds() const
{
    return static_cast<double> (settings.defaultDuration) / static_cast<double> (settings.timebase);
}

int VideoFifo::nextIndex (int pos, int offset) const
{
    jassert (offset < int (frames.size()));

    if (pos + offset >= int (frames.size()))
        return pos + offset - int (frames.size());

    return pos + offset;
}

int VideoFifo::previousIndex (int pos, int offset) const
{
    jassert (offset < int (frames.size()));

    if (pos - offset < 0)
        return int (frames.size()) + pos - offset;

    return pos - offset;
}

void VideoFifo::clear()
{
    readPosition.store (0);
    writePosition.store (0);

    for (auto& frame : frames)
    {
        frame->timecode = -1;
    }
}

void VideoFifo::dumpTimeCodes() const
{
    juce::String text ( "frames: [");
    for (auto& f : frames)
        text += " " + juce::String (f->timecode) + " ";

    text += "]";
    FOLEYS_LOG (text);
}

} // foleys
