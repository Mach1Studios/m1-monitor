#pragma once

#include "PluginProcessor.h"
#include "UI/MonitorUIBaseComponent.h"
#include <JuceHeader.h>

//==============================================================================
/**
*/
class M1MonitorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    M1MonitorAudioProcessorEditor(M1MonitorAudioProcessor&);
    ~M1MonitorAudioProcessorEditor() override;
    M1MonitorAudioProcessor* processor = nullptr;
    MonitorUIBaseComponent* monitorUIBaseComponent = nullptr;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MurImage m1logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M1MonitorAudioProcessorEditor)
};
