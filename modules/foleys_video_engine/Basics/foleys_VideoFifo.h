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

#pragma once

namespace foleys
{

/**
 The VideoFifo is a container, where the AVReader classes put the frames from reading to be displayed.
 */
class VideoFifo final
{
public:
    VideoFifo (int size);

    /**
     Returns a VideoFrame reference you can write to.
     Make sure to call finishWriting once you are done writing.
     */
    VideoFrame& getWritingFrame();

    /**
     This unlocks the videoFrame you are currently writing to and advances the write pointer.
     */
    void finishWriting();

    VideoFrame& getFrame (int64_t timecode);
    VideoFrame& getFrameSeconds (double pts);

    /**
     Sets the current timecode. This is important if you don't use the frames,
     otherwise the video fifo will fill up and clogg the reading

     @returns true if a frame is available, or false if not
     */
    bool setTimeCodeSeconds (double pts);

    /** Returns tha last written frame. Use this for streaming clips */
    VideoFrame& getLatestFrame();

    int getNumAvailableFrames() const;
    bool isFrameAvailable (double pts) const;

    /**
     Returns the number of frames that can be filled
     */
    int getFreeSpace() const;

    /**
     Reset all indices and set all VideoFrames to empty (timecode = -1)
     */
    void clear();

    /**
     Sets the VideoSettings. This is needed for the defaultDuration, but can be also used for creating the empty frames.
     */
    void setVideoSettings (const VideoStreamSettings& settings);

    /**
     For debugging: this shows all currently available timecodes in the fifo
     */
    void dumpTimeCodes() const;

    double getFrameDurationInSeconds() const;

private:
    int findFramePosition (int64_t timecode, int start) const;

    int nextIndex (int pos, int offset=1) const;
    int previousIndex (int pos, int offset=1) const;

    VideoStreamSettings     settings;

    std::vector<std::unique_ptr<VideoFrame>> frames;
    std::atomic<int>    writePosition {0};
    std::atomic<int>    readPosition  {0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoFifo)
};


} // foleys
