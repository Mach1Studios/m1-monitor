#include "M1Analytics.h"

void M1Analytics::createUsageData(const juce::String& eventName, const juce::var& properties)
{
    // Mixpanel endpoint
    juce::URL mixpanelURL("https://api.mixpanel.com/track/");

    const auto anon_id = juce::String(juce::SystemStats::getDeviceIdentifiers().joinIntoString(":").hashCode64());

    // Base properties required by Mixpanel
    juce::DynamicObject::Ptr baseProps = new juce::DynamicObject();
    baseProps->setProperty("token", secret); // Replace with your token
    baseProps->setProperty("time", juce::Time::getCurrentTime().toMilliseconds() / 1000);
    baseProps->setProperty("user_id", anon_id);
    baseProps->setProperty("$os", juce::SystemStats::getOperatingSystemName());
    baseProps->setProperty("$app_name", JucePlugin_Name);
    baseProps->setProperty("$app_version", JucePlugin_VersionString);
    baseProps->setProperty("region", juce::SystemStats::getUserRegion());
    baseProps->setProperty("cpu", juce::SystemStats::getCpuVendor());

    // Merge user-defined properties
    if (properties.isObject())
    {
        NamedValueSet& propSet = properties.getDynamicObject()->getProperties();
        for (int i = 0; i < propSet.size(); ++i)
        {
            baseProps->setProperty(propSet.getName(i), propSet.getValueAt(i));
        }
    }

    // Create the event JSON
    juce::DynamicObject::Ptr eventObj = new juce::DynamicObject();
    eventObj->setProperty("event", eventName);
    eventObj->setProperty("properties", juce::var(baseProps));

    juce::var eventVar(eventObj);
    juce::String eventJson = juce::JSON::toString(eventVar);

    // Base64 encode the JSON string
    juce::MemoryOutputStream mo;
    juce::Base64::convertToBase64(mo, eventJson.toRawUTF8(), eventJson.getNumBytesAsUTF8());
    juce::String base64Data = mo.toString();

    // Prepare POST parameters
    juce::URL::OpenStreamProgressCallback* progressCallback = nullptr;
    void* progressCallbackContext = nullptr;

    // Create URL with parameters
    url = mixpanelURL.withPOSTData("data=" + base64Data + "&verbose=1");
}

M1Analytics::M1Analytics(const juce::String& eventName) : juce::ThreadPoolJob("M1-Monitor Analytics")
{
}

juce::ThreadPoolJob::JobStatus M1Analytics::runJob()
{
    // Define custom properties
    juce::DynamicObject::Ptr props = new juce::DynamicObject();
    props->setProperty("sample_rate", "48000");
    props->setProperty("buffer_size", "512");

    var properties(props);

    // Track the event
    createUsageData("Monitor-Initialized", properties);

    // Send the request asynchronously
    juce::WebInputStream stream(url, true);
    stream.withConnectionTimeout(2000);
    stream.connect(nullptr);

    if (stream.isError())
    {
        // Handle error
        DBG("Mixpanel tracking failed.");
    }
    else
    {
        // Optionally read the response
        juce::String response = stream.readEntireStreamAsString();
        DBG("Mixpanel response: " + response);
    }

    return juce::ThreadPoolJob::jobHasFinished;
}
