/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "juce_murka/JuceMurkaBaseComponent.h"
#include "m1_orientation_client/UI/M1Label.h"
#include "m1_orientation_client/UI/M1OrientationWindowToggleButton.h"
#include "m1_orientation_client/UI/M1OrientationClientWindow.h"

//==============================================================================
/**
*/
class M1MonitorAudioProcessorEditor  : public juce::AudioProcessorEditor, public JuceMurkaBaseComponent
{
public:
    M1MonitorAudioProcessorEditor (M1MonitorAudioProcessor&);
    ~M1MonitorAudioProcessorEditor() override;

    //==============================================================================
    void initialise() override;
    void render() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    M1MonitorAudioProcessor& audioProcessor;
    
    MurImage m1logo;

    M1OrientationClientWindow orientationControlWindow;
    bool showOrientationControlMenu = false;
    bool showedOrientationControlBefore = false;
    int DEBUG_orientationDeviceSelected = -1;
    bool DEBUG_trackYaw = true, DEBUG_trackPitch = true, DEBUG_trackRoll = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (M1MonitorAudioProcessorEditor)
};
