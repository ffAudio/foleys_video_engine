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

#pragma once

namespace foleys
{

/**
 The VideoFifo is a container, where the AVReader classes put the frames from reading to be displayed.
 */
class VideoFifo final
{
public:
    VideoFifo() = default;

    void pushVideoFrame (juce::Image& image, int64_t timestamp);
    std::pair<int64_t, juce::Image> popVideoFrame();

    std::pair<int64_t, juce::Image> getVideoFrame (double timestamp) const;
    bool isFrameAvailable (double timestamp) const;

    int getNumAvailableFrames() const;
    int64_t getLowestTimeCode() const;
    int64_t getHighestTimeCode() const;

    juce::Image getOldestFrameForRecycling();

    int64_t getFrameCountForTime (double time) const;

    size_t size() const;

    void clear();

    void clearFramesOlderThan (int64_t count);

    VideoStreamSettings& getVideoSettings();

private:
    juce::CriticalSection lock;

    VideoStreamSettings settings;

    std::map<int64_t, juce::Image> videoFrames;
    int64_t lastViewedFrame = -1;
    bool reverse = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoFifo)
};


} // foleys
