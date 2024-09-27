#pragma once

#include <JuceHeader.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class M1Analytics : public juce::ThreadPoolJob
{
public:
    M1Analytics(const juce::String& _eventName = "Launched", int _sampleRate = 0, int _m1Mode = 0, bool _hasHelper = false);
    juce::ThreadPoolJob::JobStatus runJob() override;
    void createUsageData(const juce::String& eventName, const juce::var& properties);
    juce::String getMixpanelToken();

private:
    bool isAnalyticsEnabled = true;
    bool isUsingProxy = false;
    juce::URL url;
    juce::String api_key = "05b79d8d7aeeb24730569c23383e26a5";
    juce::String productName;
    juce::String eventName;
    int sampleRate = 0;
    int m1Mode = 0;
    bool hasHelper = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M1Analytics)
};
