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
 The AudioMixer is used by the ComposedClip to sum the audio of the individual clips.
 If you need alternative mixing algorithms, i.e. for advanced routing etc. you can
 override this interface and supply an instance to your ComposedClip. A default
 implementation for summing stereo tracks is available as DefaultAudioMixer.
 */
class AudioMixer
{
public:
    AudioMixer() = default;
    virtual ~AudioMixer() = default;

    virtual void setup (int numChannels, double sampleRate, int samplesPerBlockExpected) = 0;

    virtual void mixAudio (const juce::AudioSourceChannelInfo& info,
                           const int64_t position,
                           const double  timeInSeconds,
                           const std::vector<std::shared_ptr<ClipDescriptor>>& clips) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioMixer)
};

} // foleys
