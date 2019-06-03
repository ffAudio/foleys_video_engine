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

    juce::StringPairArray UsageReporter::createUsageData (const juce::String& event)
{
    const auto anon_id = juce::String (juce::SystemStats::getDeviceIdentifiers().joinIntoString (":").hashCode64());

    juce::StringPairArray data;
    data.set ("v",   "1");
    data.set ("tid", "UA-115384200-2");
    data.set ("cid", anon_id);
    data.set ("t",   "event");
    data.set ("ec",  "info");
    data.set ("ea",  event);

    data.set ("cd1", FOLEYS_ENGINE_VERSION);
    data.set ("cd2", juce::SystemStats::getJUCEVersion());
    data.set ("cd3", juce::SystemStats::getOperatingSystemName());
    data.set ("cd4", juce::SystemStats::getDeviceDescription());
    data.set ("cd5", anon_id);

    juce::String appType, appName, appVersion, appManufacturer;

#if defined(JucePlugin_Name)
    appType         = "Plugin";
    appName         = JucePlugin_Name;
    appVersion      = JucePlugin_VersionString;
    appManufacturer = JucePlugin_Manufacturer;
#else
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        appType = "Application";

        if (auto* app = juce::JUCEApplicationBase::getInstance())
        {
            appName    = app->getApplicationName();
            appVersion = app->getApplicationVersion();
        }
    }
    else
    {
        appType = "Library";
    }
#endif

    data.set ("cd5", appType);
    data.set ("cd6", appName);
    data.set ("cd7", appVersion);
    data.set ("cd8", appManufacturer);

    data.set ("an", appName);
    data.set ("av", appVersion);

    return data;
}

UsageReporter::UsageReporter (const juce::String& event) : juce::ThreadPoolJob ("Engine Usage Report")
{
    const auto address = "https://www.google-analytics.com/collect";
    auto agentCPUVendor = juce::SystemStats::getCpuVendor();

    if (agentCPUVendor.isEmpty())
        agentCPUVendor = "CPU";

    auto agentOSName = juce::SystemStats::getOperatingSystemName().replaceCharacter ('.', '_')
    .replace ("iOS", "iPhone OS");
#if JUCE_IOS
    agentOSName << " like Mac OS X";
#endif

    userAgent << "Mozilla/5.0 ("
    << juce::SystemStats::getDeviceDescription() << ";"
    << agentCPUVendor << " " << agentOSName << ";"
    << juce::SystemStats::getDisplayLanguage() << ")";

    juce::StringArray postData;

    auto parameters = createUsageData (event);
    for (auto& key : parameters.getAllKeys())
        if (parameters[key].isNotEmpty())
            postData.add (key + "=" + juce::URL::addEscapeChars (parameters[key], true));

    url = juce::URL (address).withPOSTData (postData.joinIntoString ("&"));
}

juce::ThreadPoolJob::JobStatus UsageReporter::runJob()
{
    auto webStream = std::make_unique<juce::WebInputStream> (url, true);
    webStream->withExtraHeaders (headers);
    webStream->connect (nullptr);

    return juce::ThreadPoolJob::jobHasFinished;
}

} // namespace foleys
