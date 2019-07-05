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
 The VideoProcessor is the base class to implement any video processor.
 It offers an interface, how the VideoMixer will call supplying Images.
 Similar to the AudioProcessor, you can add ProcessorParameters to control
 the behaviour of the VideoProcessor.
 */
class VideoProcessor
{
public:

    VideoProcessor() = default;

    virtual ~VideoProcessor() = default;

    /** Override this method to return a human readable name to identify the processor later */
    virtual const juce::String getName() const = 0;

    /**
     Override this method to implement the actual video processing. The processing is done in place. If you need a copy of the frame,
     it's best to keep an empty frame as member, where you copy the original before processing and
     process from the copy into the original.

     @param output is the image to write into
     @param input is the original image to read from
     @param count this is the frame counter in settings.timebase counts
     @param settings is the output settings that you will produce. You find the size and the timebase here
     @param clipDuration is the duration of the clip. This is especially handy for creating transitions
     */
    virtual void processFrame (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) = 0;

    virtual std::vector<ProcessorParameter*> getParameters() = 0;

    virtual void getStateInformation (juce::MemoryBlock& destData) = 0;

    virtual void setStateInformation (const void* data, int sizeInBytes) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoProcessor)
};

} // foleys
