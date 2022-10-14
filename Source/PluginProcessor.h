/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Mach1Decode.h>
#include "Transport.h"

//==============================================================================
/**
*/
class M1MonitorAudioProcessor  : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    M1MonitorAudioProcessor();
    ~M1MonitorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterChanged(const juce::String &parameterID, float newValue) override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processStereoDownmix (juce::AudioBuffer<float>&);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Parameter Setup
    juce::AudioProcessorValueTreeState& getValueTreeState();
    static juce::String paramYaw;
    static juce::String paramPitch;
    static juce::String paramRoll;
    static juce::String paramYawEnable;
    static juce::String paramPitchEnable;
    static juce::String paramRollEnable;
    static juce::String paramMonitorMode;
    static juce::String paramDecodeMode;
    
    Mach1Decode m1Decode;
    juce::PluginHostType hostType;

    void setStatus(bool success, std::string message);
    M1OrientationOSCClient m1OrientationOSCClient;
    
    // Transport
    ScopedPointer<Transport> transport;
    
private:
    juce::UndoManager mUndoManager;
    juce::AudioProcessorValueTreeState parameters;

    std::vector<float> spatialMixerCoeffs;
    std::vector<juce::LinearSmoothedValue<float>> smoothedChannelCoeffs;
    juce::AudioBuffer<float> tempBuffer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (M1MonitorAudioProcessor)
};
