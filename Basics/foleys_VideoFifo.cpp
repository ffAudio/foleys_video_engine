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

VideoFifo::VideoFifo()
{
}

VideoFrame& VideoFifo::getWritingFrame()
{
    auto pos = size_t (writePosition.load());

    if (pos + 1 >= frames.size())
        writePosition.store (0);
    else
        ++writePosition;

    return frames[pos];
}

const VideoFrame& VideoFifo::getFrame (int64_t timecode)
{
    auto pos = readPosition.load();
    auto nextPos = findFramePosition (timecode, pos);
    if (nextPos >= 0)
    {
        readPosition.store (nextPos);
        return frames [size_t (nextPos)];
    }

    return frames [size_t (pos)];
}

const VideoFrame& VideoFifo::getFrameSeconds (double pts)
{
    auto timecode = convertTimecode (pts, settings);
    return getFrame (timecode);
}

int VideoFifo::getNumAvailableFrames() const
{
    auto read = readPosition.load();
    auto write = writePosition.load();

    return (write > read) ? write - read : int (frames.size()) + (write - read);
}

bool VideoFifo::isFrameAvailable (double pts) const
{
    auto timecode = convertTimecode (pts, settings);
    auto pos = findFramePosition (timecode, readPosition.load());
    return pos >= 0;
}

int VideoFifo::findFramePosition (int64_t timecode, int start) const
{
    const auto size = int (frames.size());

    // direct hit
    if (juce::isPositiveAndBelow (timecode - frames [size_t (start)].timecode, settings.defaultDuration))
        return start;

    // forward seek
    while (timecode >= frames [size_t (start)].timecode + settings.defaultDuration)
    {
        start = (start + 1 < size) ? start + 1 : 0;
        if (frames [size_t (start)].timecode < 0)
            return -1;

        if (juce::isPositiveAndBelow (timecode - frames [size_t (start)].timecode, settings.defaultDuration))
            return start;
    }

    // backward seek
    while (timecode >= frames [size_t (start)].timecode + settings.defaultDuration)
    {
        start = (start - 1 < 0) ? size - 1 : start - 1;
        if (frames [size_t (start)].timecode < 0)
            return -1;

        if (juce::isPositiveAndBelow (timecode - frames [size_t (start)].timecode, settings.defaultDuration))
            return start;
    }

    return -1;
}

void VideoFifo::setVideoSettings (VideoStreamSettings& s)
{
    settings = s;
}

void VideoFifo::clear()
{
    readPosition.store (0);
    writePosition.store (0);

    for (auto& frame : frames)
    {
        frame.timecode = -1;
#if FOLEYS_USE_OPENGL
        frame.upToDate = false;
#endif
    }
}

} // foleys
