#pragma once

#include <JuceHeader.h>

class M1Analytics : public juce::ThreadPoolJob
{
public:
    M1Analytics(const juce::String& event = "MonitorLaunch");
    juce::ThreadPoolJob::JobStatus runJob() override;
    static juce::StringPairArray createUsageData(const juce::String& event);

private:
    juce::String userAgent;
    juce::URL url;
    juce::String headers;
    juce::String apiKey = "G-CP8900GQKP";
    juce::String secret = "xkPfUnMKSVCNxqhnf75uHQ";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M1Analytics)
};
