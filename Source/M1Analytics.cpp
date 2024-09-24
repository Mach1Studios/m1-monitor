#include "M1Analytics.h"

juce::StringPairArray M1Analytics::createUsageData(const juce::String& event)
{
    const auto anon_id = juce::String(juce::SystemStats::getDeviceIdentifiers().joinIntoString(":").hashCode64());

    juce::StringPairArray data;
    data.set("v", "1");
    data.set("tid", "UA-145097104-1");
    data.set("cid", anon_id);
    data.set("t", "event");
    data.set("ec", "info");
    data.set("ea", event);

    data.set("cd1", juce::SystemStats::getUserRegion());
    data.set("cd2", juce::SystemStats::getOperatingSystemName());
    data.set("cd3", juce::SystemStats::getDeviceDescription());
    data.set("cd4", anon_id);

    juce::String appType, appName, appVersion, appManufacturer;

#if defined(JucePlugin_Name)
    appType = "Plugin";
    appName = JucePlugin_Name;
    appVersion = JucePlugin_VersionString;
    appManufacturer = JucePlugin_Manufacturer;
#else
    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        appType = "Application";

        if (auto* app = juce::JUCEApplicationBase::getInstance())
        {
            appName = app->getApplicationName();
            appVersion = app->getApplicationVersion();
        }
    }
    else
    {
        appType = "Library";
    }
#endif

    data.set("cd5", appType);
    data.set("cd6", appName);
    data.set("cd7", appVersion);
    data.set("cd8", appManufacturer);

    data.set("an", appName);
    data.set("av", appVersion);

    return data;
}

M1Analytics::M1Analytics(const juce::String& event) : juce::ThreadPoolJob("M1-Monitor Analytics")
{
    const auto address = "https://www.google-analytics.com/collect";
    auto agentCPUVendor = juce::SystemStats::getCpuVendor();

    if (agentCPUVendor.isEmpty())
        agentCPUVendor = "CPU";

    auto agentOSName = juce::SystemStats::getOperatingSystemName().replaceCharacter('.', '_').replace("iOS", "iPhone OS");
#if JUCE_IOS
    agentOSName << " like Mac OS X";
#endif

    userAgent << "Mozilla/5.0 ("
              << juce::SystemStats::getDeviceDescription() << ";"
              << agentCPUVendor << " " << agentOSName << ";"
              << juce::SystemStats::getDisplayLanguage() << ")";

    juce::StringArray postData;

    auto parameters = createUsageData(event);
    for (auto& key : parameters.getAllKeys())
        if (parameters[key].isNotEmpty())
            postData.add(key + "=" + juce::URL::addEscapeChars(parameters[key], true));

    url = juce::URL(address).withPOSTData(postData.joinIntoString("&"));
}

juce::ThreadPoolJob::JobStatus M1Analytics::runJob()
{
    juce::WebInputStream(url, true)
        .withExtraHeaders(headers)
        .withConnectionTimeout(2000)
        .connect(nullptr);

    return juce::ThreadPoolJob::jobHasFinished;
}
