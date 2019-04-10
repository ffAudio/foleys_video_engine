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

void VideoFifo::pushVideoFrame (juce::Image& image, juce::int64 timestamp)
{
    const juce::ScopedLock sl (lock);
    videoFrames [timestamp] = image;
}

juce::Image VideoFifo::getVideoFrame (double timestamp) const
{
    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (timestamp / timebase);
    if (vf != videoFrames.end())
    {
        const_cast<juce::int64&>(lastViewedFrame) = vf->first;
        return vf->second;
    }

    return {};
}

Timecode VideoFifo::getFrameTimecodeForTime (double time) const
{
    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (time / timebase);
    if (vf != videoFrames.end())
        return {vf->first, timebase};

    return {};
}

int VideoFifo::getNumAvailableFrames() const
{
    const juce::ScopedLock sl (lock);

    auto it = videoFrames.find (lastViewedFrame);
    return int (std::distance (it, videoFrames.end()));
}

juce::int64 VideoFifo::getLowestTimeCode() const
{
    const juce::ScopedLock sl (lock);

    if (videoFrames.empty())
        return 0;

    return videoFrames.cbegin()->first;
}

juce::int64 VideoFifo::getHighestTimeCode() const
{
    const juce::ScopedLock sl (lock);

    if (videoFrames.empty())
        return 0;

    auto it = videoFrames.end();
    --it;
    return it->first;
}

juce::Image VideoFifo::getOldestFrameForRecycling()
{
    const juce::ScopedLock sl (lock);

    juce::Image image;

    if (reverse == false)
    {
        auto iterator = videoFrames.begin();
        if (iterator != videoFrames.end() && iterator->first < lastViewedFrame)
        {
            image = iterator->second;
            videoFrames.erase (iterator);
        }
    }
    else if (! videoFrames.empty())
    {
        auto iterator = videoFrames.end();
        --iterator;
        if (iterator->first > lastViewedFrame)
        {
            image = iterator->second;
            videoFrames.erase (iterator);
        }
    }

    if (image.isNull())
        image = juce::Image (juce::Image::ARGB, originalSize.width, originalSize.height, false);

    return image;
}

void VideoFifo::clear()
{
    const juce::ScopedLock sl (lock);

    videoFrames.clear();
    lastViewedFrame = 0;
}

void VideoFifo::clearFramesOlderThan (Timecode timecode)
{
    const juce::ScopedLock sl (lock);

    auto current = videoFrames.find (timecode.count);
    if (current == videoFrames.begin())
        return;

    videoFrames.erase (videoFrames.begin(), --current);
}

void VideoFifo::setTimebase (double timebaseToUse)
{
    timebase = timebaseToUse;
}

void VideoFifo::setSize (Size size)
{
    originalSize = size;
}

} // foleys
