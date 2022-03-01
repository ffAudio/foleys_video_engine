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
     This class mixes the individual clips in ComposedClip. It uses the software
     backend of JUCE.
     */
class SoftwareVideoMixer : public VideoMixer
{
public:
    SoftwareVideoMixer() = default;

    /**
     The ComposedClip will call this to let you compose the various clips.
     @param target is the image to render into
     @param settings are the stream settings of the produced stream
     @param count is the frame counter in settings.timebase
     @param timeInSeconds is the current time in seconds, since the originating stream
            has not necessarily the same timebase
     @param clips is a vector of ClipDescriptors. Each ClipDescriptor has it's own list of processors.
     */
    void compose (juce::Image& target,
                  VideoStreamSettings settings,
                  int64_t count,
                  double  timeInSeconds,
                  const   std::vector<std::shared_ptr<ClipDescriptor>>& clips) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftwareVideoMixer)
};

} // foleys
