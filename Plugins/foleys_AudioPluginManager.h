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
 The AudioPluginManager is used to create AudioProcessor instances to be placed into
 the audio processing pipeline. You can add support for 3rd party plugins here.
 */
class AudioPluginManager
{
public:
    AudioPluginManager();

    /**
     Register a factory to return an AudioProcessor instance from an identifierString. This is used for built in
     AudioProcessors.
     */
    void registerAudioProcessor (const juce::String& identifierString, std::function<std::unique_ptr<juce::AudioProcessor>()>);

    /**
     Creates an actual instance of an AudioPlugin. It will try the first the built in factories, and then the
     AudioPlugins that were found on the system.
     */
    std::unique_ptr<juce::AudioProcessor> createAudioPluginInstance (const juce::String& identifierString,
                                                                     double sampleRate,
                                                                     int blockSize,
                                                                     juce::String& error) const;

private:

    std::map<juce::String, std::function<std::unique_ptr<juce::AudioProcessor>()>> factories;

    juce::KnownPluginList          knownPluginList;
    juce::AudioPluginFormatManager pluginManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginManager)
};

}
