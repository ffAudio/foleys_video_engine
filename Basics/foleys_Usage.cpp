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
    juce::WebInputStream (url, true)
    .withExtraHeaders (headers)
    .withConnectionTimeout (2000)
    .connect (nullptr);

    return juce::ThreadPoolJob::jobHasFinished;
}

//==============================================================================

FoleysSplashScreen::FoleysSplashScreen()
{
    setOpaque (false);
    static const unsigned char logo[] =
    "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
    "<svg version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\" width=\"472px\" height=\"469px\" viewBox=\"0 0 4720 4690\" preserveAspectRatio=\"xMidYMid meet\">\n"
    "<g id=\"layer101\" fill=\"#4b3427\" stroke=\"none\">\n"
    " <path d=\"M2250 4580 c-66 -35 -501 -402 -790 -666 -394 -361 -793 -786 -1194 -1274 -151 -183 -168 -213 -168 -289 0 -95 -2 -91 283 -416 322 -367 607 -667 921 -971 334 -324 880 -809 950 -845 47 -24 146 -26 191 -3 36 19 79 54 297 244 516 449 1031 958 14"
    "81 1465 227 254 340 390 360 427 26 50 26 146 0 196 -21 41 -225 278 -499 582 -244 270 -831 855 -1107 1102 -325 291 -484 426 -527 449 -50 25 -149 25 -198 -1z\"/>\n"
    " </g>\n"
    "<g id=\"layer102\" fill=\"#2eb3d5\" stroke=\"none\">\n"
    " <path d=\"M2325 3896 c-475 -396 -1146 -1068 -1508 -1508 -48 -58 -87 -110 -87 -115 0 -11 218 -228 223 -222 2 2 44 54 92 114 235 294 434 516 734 818 243 246 599 567 627 567 5 0 59 -50 119 -110 l110 -110 -518 -518 -517 -517 110 -110 110 -110 513 513 c2"
    "81 281 517 512 522 512 5 0 116 -106 245 -235 l235 -235 110 110 110 110 -565 565 -565 565 -100 -84z\"/>\n"
    " <path d=\"M3675 2563 c-254 -317 -475 -563 -770 -859 -207 -207 -509 -483 -583 -532 -23 -15 -26 -13 -135 96 l-112 112 507 508 508 507 -112 113 -112 113 -508 -508 -508 -508 -240 240 -240 240 -112 -113 -113 -112 567 -567 567 -566 117 99 c331 278 975 905"
    " 1250 1214 101 114 280 326 321 381 10 13 -6 33 -101 128 l-113 112 -78 -98z\"/>\n"
    " </g>\n"
    "\n"
    "</svg>\n";

    foleys = juce::Drawable::createFromImageData (logo, 1456);

    juce::Timer::callAfterDelay (6000, [this] { setVisible (false); } );
}

void FoleysSplashScreen::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour (juce::Colours::darkgrey.withAlpha (0.8f));
    g.fillRoundedRectangle (area.toFloat(), 8.0f);
    g.setColour (juce::Colours::silver);
    g.drawRoundedRectangle (area.toFloat().reduced (3.0f), 6.0f, 1.0f);
    area.reduce (5, 5);
    g.drawFittedText ("powered by", area.removeFromTop (20), juce::Justification::centred, 1);
    g.drawFittedText ("click to visit", area.removeFromBottom (14), juce::Justification::centred, 1);
    foleys->drawWithin (g, area.removeFromLeft (area.getHeight()).toFloat(), juce::RectanglePlacement (juce::RectanglePlacement::stretchToFit), 1.0f);
    g.setFont (18.0f);
    g.drawFittedText ("Foleys Finest\nAudio Software", area, juce::Justification::centred, 2);
}

void FoleysSplashScreen::mouseDown (const juce::MouseEvent&)
{
    juce::URL ("https://foleysfinest.com/").launchInDefaultBrowser();
}


} // namespace foleys
