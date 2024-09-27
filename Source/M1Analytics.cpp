#include "M1Analytics.h"

void M1Analytics::createUsageData(const juce::String& eventName, const juce::var& properties)
{
    // HTTP POST endpoint
    juce::URL endpointURL;
    if (isUsingProxy) {
        // Proxy server endpoint
        juce::URL proxy_ep("http://your-ec2-ip-address:3000/track");
        endpointURL = proxy_ep;
    } else {
        juce::URL mp_ep("https://api.mixpanel.com/track/");
        endpointURL = mp_ep;
    }

    const auto anon_id = juce::String(juce::SystemStats::getDeviceIdentifiers().joinIntoString(":").hashCode64());

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
#endif
    // Base properties required by Mixpanel
    juce::DynamicObject::Ptr baseProps = new juce::DynamicObject();
    baseProps->setProperty("token", api_key);
    baseProps->setProperty("time", juce::Time::getCurrentTime().toMilliseconds() / 1000);
    baseProps->setProperty("distinct_id", anon_id);
    baseProps->setProperty("$os", juce::SystemStats::getOperatingSystemName());
    baseProps->setProperty("$app_type", appType);
    baseProps->setProperty("$app_name", appName);
    baseProps->setProperty("$app_version", appVersion);
    baseProps->setProperty("$app_manu", appManufacturer);
    baseProps->setProperty("m1Mode", m1Mode);
    baseProps->setProperty("hasHelper", hasHelper);
    baseProps->setProperty("$region", juce::SystemStats::getUserRegion());
    baseProps->setProperty("language", juce::SystemStats::getUserLanguage());
    baseProps->setProperty("cpu", juce::SystemStats::getCpuVendor());
    baseProps->setProperty("cpu-model", juce::SystemStats::getCpuModel());
  
    // TODO: implement this safely (this is not in a mesage thread and therefore not thread safe)
    /*
    juce::String res = "Unknown";
    if (const juce::Displays::Display* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        juce::Rectangle<int> r = d->totalArea;
        res = juce::String(r.getWidth()) + "x" + juce::String(r.getHeight());
    }
    baseProps->setProperty("display", res);
     */

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
    url = endpointURL.withPOSTData("data=" + base64Data + "&verbose=1");
}

juce::String M1Analytics::getMixpanelToken()
{
    // Try to read from an environment variable
    const char* envVar = std::getenv("MIXPANEL_PROJECT_TOKEN");
    if (envVar != nullptr)
        return juce::String(envVar);

    // Alternatively, read from a config file
    // Using common support files installation location
    juce::File configFile;
    juce::File m1SupportDirectory = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);
    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
        // test for any mac OS
        configFile = m1SupportDirectory.getChildFile("Application Support").getChildFile("Mach1");
    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
        // test for any windows OS
        configFile = m1SupportDirectory.getChildFile("Mach1");
    } else {
        configFile = m1SupportDirectory.getChildFile("Mach1");
    }
    configFile = configFile.getChildFile("settings.json");
    DBG("Opening settings file: " + configFile.getFullPathName().quoted());
    
    if (configFile.existsAsFile()) {
        juce::String jsonString = configFile.loadFileAsString();
        juce::var jsonData = juce::JSON::parse(jsonString);

        if (jsonData.isVoid())
        {
            DBG("Failed to parse settings.json");
            // Token not found
            return juce::String();
        }
    
        if (jsonData.isObject())
        {
            juce::var apiKeyVar = jsonData.getProperty("mp_api_key", var());

            if (apiKeyVar.isString())
            {
                return apiKeyVar.toString();
            }
            else
            {
                DBG("mp_api_key is missing or not a string.");
            }
        }
        else
        {
            DBG("JSON data is not an object.");
        }
    }

    // Token not found
    return juce::String();
}

M1Analytics::M1Analytics(const juce::String& _eventName, int _sampleRate, int _m1Mode, bool _hasHelper) : juce::ThreadPoolJob("M1Analytics")
{
    eventName = _eventName;
    sampleRate = _sampleRate;
    m1Mode = _m1Mode;
    hasHelper = _hasHelper;
}

juce::ThreadPoolJob::JobStatus M1Analytics::runJob()
{
    if (!isAnalyticsEnabled)
        return juce::ThreadPoolJob::jobHasFinished;

    // grab the hardcoded api_key first, otherwise look for an installed key
    if (api_key.isEmpty()) {
        juce::String projectToken = getMixpanelToken();
        if (projectToken.isEmpty())
        {
            DBG("Mixpanel project token not found.");
            return juce::ThreadPoolJob::jobHasFinished;
        }
    }

    // Define custom properties
    juce::DynamicObject::Ptr props = new juce::DynamicObject();
    props->setProperty("sample_rate", sampleRate);

    juce::var properties(props);

    // Track the event
    createUsageData(eventName, properties);

    // Send the request asynchronously
    juce::WebInputStream stream(url, true);
    if (isUsingProxy) {
        stream.withExtraHeaders("Content-Type: application/json"); // needed for proxy usage
    }
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
