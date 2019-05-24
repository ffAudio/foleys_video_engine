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

void DefaultAudioMixer::setup (int numChannels, double sampleRate, int samplesPerBlockExpected)
{
    mixBuffer.setSize (numChannels, samplesPerBlockExpected);
}

void DefaultAudioMixer::mixAudio (const juce::AudioSourceChannelInfo& info,
                                  const int64_t position,
                                  const double  timeInSeconds,
                                  const std::vector<std::shared_ptr<ClipDescriptor>>& clips)
{
    for (auto& clip : clips)
    {
        const auto start = clip->getStartInSamples();
        if (position + info.numSamples >= start && position < start + clip->getLengthInSamples())
        {
            const auto offset = std::max (int (start - position), 0);
            juce::AudioSourceChannelInfo reader (&mixBuffer, 0, info.numSamples - offset);
            clip->clip->getNextAudioBlock (reader);

            juce::AudioBuffer<float> procBuffer (mixBuffer.getArrayOfWritePointers(), mixBuffer.getNumChannels(), 0, info.numSamples - offset);
            juce::MidiBuffer midiDummy;
            for (auto& controller : clip->audioProcessors)
            {
                controller->updateAutomation ((timeInSeconds - clip->getStart()) + clip->getOffset());
                if (auto* audioProcessor = controller->getAudioProcessor())
                    if (! audioProcessor->isSuspended())
                        audioProcessor->processBlock (procBuffer, midiDummy);
            }

            for (int channel = 0; channel < mixBuffer.getNumChannels(); ++channel)
                info.buffer->addFrom (channel, info.startSample + offset, mixBuffer.getReadPointer (channel), info.numSamples - offset);
        }
    }
}

};
