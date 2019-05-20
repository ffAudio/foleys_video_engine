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

class VideoProcessor  : public juce::ControllableProcessorBase
{
public:

    VideoProcessor() = default;

    virtual ~VideoProcessor() = default;

    /**
     Override this method to implement the actual video processing.
     @param frame this is the image of the frame
     @param count this is the frame counter in settings.timebase counts
     @param settings is the output settings that you will produce. You find the size and the timebase here
     @param clipDuration is the duration of the clip. This is especially handy for creating transitions
     */
    virtual void processFrameReplacing (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) = 0;

    /**
     Override this method to implement the actual video processing. The default implementation creates a copy and calls processFrameReplacing.
     But for some processors you might want to save the copy step for performance reasons.
     @param output is the image to write into
     @param input is the original image to read from
     @param count this is the frame counter in settings.timebase counts
     @param settings is the output settings that you will produce. You find the size and the timebase here
     @param clipDuration is the duration of the clip. This is especially handy for creating transitions
     */
    virtual void processFrame (juce::Image& output, const juce::Image& input, int64_t count, const VideoStreamSettings& settings, double clipDuration)
    {
        output = input.createCopy();
        processFrameReplacing (output, count, settings, clipDuration);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoProcessor)
};

}
