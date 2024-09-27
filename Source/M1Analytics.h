#pragma once

#include <JuceHeader.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

class M1Analytics : public juce::ThreadPoolJob
{
public:
    M1Analytics(const juce::String& event = "MonitorLaunch");
    juce::ThreadPoolJob::JobStatus runJob() override;
    void createUsageData(const juce::String& eventName, const juce::var& properties);

private:
    juce::URL url;
    juce::String secret = "05b79d8d7aeeb24730569c23383e26a5";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M1Analytics)
};
