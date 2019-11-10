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

AudioPluginManager::AudioPluginManager (VideoEngine& videoEngineToUse)
  : videoEngine (videoEngineToUse)
{
    pluginManager.addDefaultFormats();
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

    std::unique_ptr<juce::AudioPluginInstance> plugin (pluginManager.createPluginInstance (*description, sampleRate, blockSize, error));
    return plugin;
}

juce::Array<juce::PluginDescription> AudioPluginManager::getKnownPluginDescriptions() const
{
    return knownPluginList.getTypes();
}

void AudioPluginManager::setPluginDataFile (const juce::File& file)
{
    pluginDataFile = file;

    auto xml = juce::XmlDocument::parse (pluginDataFile);
    if (xml.get() != nullptr)
    {
        knownPluginList.recreateFromXml (*xml);
    }

#if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
    videoEngine.addJob (new PluginScanJob (*this, std::make_unique<juce::AudioUnitPluginFormat>()), true);
#endif

#if JUCE_PLUGINHOST_LADSPA && JUCE_LINUX
    videoEngine.addJob (new PluginScanJob (*this, std::make_unique<juce::LADSPAPluginFormat>()), true);
#endif

#if JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS)
    videoEngine.addJob (new PluginScanJob (*this, std::make_unique<juce::VST3PluginFormat>()), true);
#endif
}

//==============================================================================

AudioPluginManager::PluginScanJob::PluginScanJob (AudioPluginManager& ownerToUse,
                                                  std::unique_ptr<juce::AudioPluginFormat>&& formatToUse)
  : juce::ThreadPoolJob ("Plugin Scanner"),
    owner (ownerToUse),
    format (std::move (formatToUse))
{
}

juce::ThreadPoolJob::JobStatus AudioPluginManager::PluginScanJob::runJob()
{
    auto deadMansPedal = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (format->getName() + "ScanPlugins.tmp");
    juce::PluginDirectoryScanner scanner (owner.knownPluginList,
                                          *format,
                                          format->getDefaultLocationsToSearch(),
                                          true,
                                          deadMansPedal);

    juce::String name;
    while (scanner.scanNextFile (true, name))
    {
        FOLEYS_LOG ("Scanning " + format->getName() + ": " + name);
    }

    auto xml = owner.knownPluginList.createXml();
    if (xml.get() != nullptr)
        xml->writeTo (owner.pluginDataFile);

    return juce::ThreadPoolJob::jobHasFinished;
}


} // foleys
