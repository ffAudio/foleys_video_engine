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

AudioPluginManager::AudioPluginManager()
{
    pluginManager.addDefaultFormats();

    registerAudioProcessor ("BUILTIN: " + PanningAudioProcessor::getPluginName(), [] { return std::make_unique<PanningAudioProcessor>(); });
}

void AudioPluginManager::registerAudioProcessor (const juce::String& identifierString, std::function<std::unique_ptr<juce::AudioProcessor>()> factory)
{
    factories [identifierString] = std::move (factory);
}

std::unique_ptr<juce::AudioProcessor> AudioPluginManager::createAudioPluginInstance (const juce::String& identifierString,
                                                                                     double sampleRate,
                                                                                     int blockSize,
                                                                                     juce::String& error) const
{
    auto factory = factories.find (identifierString);
    if (factory != factories.cend())
        return factory->second();

    auto description = knownPluginList.getTypeForIdentifierString (identifierString);
    if (description.get() == nullptr)
    {
        error = NEEDS_TRANS ("Plugin not known");
        return {};
    }

    std::unique_ptr<juce::AudioPluginInstance> plugin;
    plugin.reset (pluginManager.createPluginInstance (*description, sampleRate, blockSize, error));
    return plugin;
}

}
