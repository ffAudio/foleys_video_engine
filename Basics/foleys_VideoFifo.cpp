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

void VideoFifo::pushVideoFrame (juce::Image& image, int64_t timestamp)
{
    const juce::ScopedLock sl (lock);
    videoFrames [timestamp] = image;
}

std::pair<int64_t, juce::Image> VideoFifo::popVideoFrame()
{
    const juce::ScopedLock sl (lock);
    if (videoFrames.empty())
        return {};

    auto frame = videoFrames.extract (videoFrames.begin());
    return { frame.key(), frame.mapped() };
}

std::pair<int64_t, juce::Image> VideoFifo::getVideoFrame (double timestamp) const
{
    const juce::int64 pts = timestamp * settings.timebase;

    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (timestamp * settings.timebase);
    if (vf == videoFrames.end())
        return {};

    if (vf->first > pts && vf != videoFrames.begin())
        --vf;

    const_cast<int64_t&>(lastViewedFrame) = vf->first;
    return { vf->first, vf->second };
}

bool VideoFifo::isFrameAvailable (double timestamp) const
{
    const juce::int64 pts = timestamp * settings.timebase;

    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (pts);
    if (vf == videoFrames.end())
        return false;

    if (vf->first > pts && vf != videoFrames.begin())
        --vf;

    return vf->first <= pts && vf->first + settings.defaultDuration > pts;
}

int64_t VideoFifo::getFrameCountForTime (double timestamp) const
{
    const juce::int64 pts = timestamp * settings.timebase;

    const juce::ScopedLock sl (lock);

    auto vf = videoFrames.lower_bound (pts);
    if (vf == videoFrames.end())
        return -1;

    if (vf->first > pts && vf != videoFrames.begin())
        --vf;

    return vf->first
    ;
}

size_t VideoFifo::size() const
{
    const juce::ScopedLock sl (lock);
    return videoFrames.size();
}

int VideoFifo::getNumAvailableFrames() const
{
    const juce::ScopedLock sl (lock);

    auto it = videoFrames.lower_bound (lastViewedFrame);
    if (it == videoFrames.end())
        return 0;

    return int (std::distance (it, videoFrames.end()));
}

int64_t VideoFifo::getLowestTimeCode() const
{
    const juce::ScopedLock sl (lock);

    if (videoFrames.empty())
        return 0;

    return videoFrames.cbegin()->first;
}

int64_t VideoFifo::getHighestTimeCode() const
{
    const juce::ScopedLock sl (lock);

    if (videoFrames.empty())
        return -settings.defaultDuration;

    auto it = videoFrames.end();
    --it;
    return it->first;
}

juce::Image VideoFifo::getOldestFrameForRecycling()
{
    const juce::ScopedLock sl (lock);

    if (framesPool.size() > 0)
    {
        auto image = framesPool.back();
        framesPool.pop_back();
        return image;
    }

    if (reverse == false)
    {
        auto iterator = videoFrames.begin();
        if (iterator != videoFrames.end() && iterator->first < lastViewedFrame)
        {
            auto image = iterator->second;
            videoFrames.erase (iterator);
            return image;
        }
    }
    else if (! videoFrames.empty())
    {
        auto iterator = videoFrames.end();
        --iterator;
        if (iterator->first > lastViewedFrame)
        {
            auto image = iterator->second;
            videoFrames.erase (iterator);
            return image;
        }
    }

    return juce::Image (juce::Image::ARGB, settings.frameSize.width, settings.frameSize.height, false);
}

void VideoFifo::clear()
{
    const juce::ScopedLock sl (lock);

    for (auto it : videoFrames)
        framesPool.push_back (it.second);

    videoFrames.clear();
    lastViewedFrame = -1;
}

VideoStreamSettings& VideoFifo::getVideoSettings()
{
    return settings;
}

const VideoStreamSettings& VideoFifo::getVideoSettings() const
{
    return settings;
}

} // foleys
