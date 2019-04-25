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

std::pair<juce::int64, juce::Image> VideoFifo::popVideoFrame()
{
    const juce::ScopedLock sl (lock);
    if (videoFrames.empty())
        return {};

    auto frame = videoFrames.extract (videoFrames.begin());
    return { frame.key(), frame.mapped() };
}

juce::Image VideoFifo::getVideoFrame (double timestamp) const
{
    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (timestamp * settings.timebase);
    if (vf != videoFrames.end())
    {
        const_cast<juce::int64&>(lastViewedFrame) = vf->first;
        return vf->second;
    }

    return {};
}

bool VideoFifo::isFrameAvailable (double timestamp) const
{
    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (timestamp * settings.timebase);
    if (vf != videoFrames.end())
    {
        auto frameEnd = double (vf->first + settings.defaultDuration) / settings.timebase;
        return timestamp <= frameEnd;
    }

    return false;
}

Timecode VideoFifo::getFrameTimecodeForTime (double time) const
{
    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (time * settings.timebase);
    if (vf != videoFrames.end())
        return { vf->first, double (settings.timebase) };

    return {};
}

size_t VideoFifo::size() const
{
    return videoFrames.size();
}

int VideoFifo::getNumAvailableFrames() const
{
    const juce::ScopedLock sl (lock);

    auto it = videoFrames.lower_bound (lastViewedFrame);
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
        image = juce::Image (juce::Image::ARGB, settings.frameSize.width, settings.frameSize.height, false);

    return image;
}

void VideoFifo::clear (juce::int64 count)
{
    const juce::ScopedLock sl (lock);

    videoFrames.clear();
    lastViewedFrame = count;
}

void VideoFifo::clearFramesOlderThan (Timecode timecode)
{
    const juce::ScopedLock sl (lock);

    auto current = videoFrames.find (timecode.count);
    if (current == videoFrames.begin())
        return;

    videoFrames.erase (videoFrames.begin(), --current);
}

VideoStreamSettings& VideoFifo::getVideoSettings()
{
    return settings;
}

} // foleys
