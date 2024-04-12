/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <Mach1Decode.h>
#include "Config.h"
#include "TypesForDataExchange.h"
#include "Transport.h"
#include "MonitorOSC.h"

//==============================================================================
/**
*/
class M1MonitorAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener, public juce::Timer
{
public:
    //==============================================================================
    M1MonitorAudioProcessor();
    ~M1MonitorAudioProcessor() override;

    static AudioProcessor::BusesProperties getHostSpecificLayout() {
        // This determines the initial bus i/o for plugin on construction and depends on the `isBusesLayoutSupported()`
        juce::PluginHostType hostType;
        
        // Multichannel Pro Tools
        // TODO: Check if Ultimate/HD
        if (hostType.isProTools()) {
            return BusesProperties()
                .withInput("Mach1Spatial Input", juce::AudioChannelSet::create7point1(), true)
                .withOutput("Default Output", juce::AudioChannelSet::stereo(), true);
        }
        
        // Multichannel DAWs
        if (hostType.isReaper() || hostType.isNuendo() || hostType.isDaVinciResolve() || hostType.isArdour()) {
            if (hostType.getPluginLoadedAs() == AudioProcessor::wrapperType_VST3) {
                return BusesProperties()
                // VST3 requires named plugin configurations only
                .withInput("Mach1Spatial In", juce::AudioChannelSet::ambisonic(5), true) // 36 named channel
                .withOutput("Stereo Out", juce::AudioChannelSet::stereo(), true);
            } else {
                return BusesProperties()
                .withInput("Mach1Spatial In", juce::AudioChannelSet::discreteChannels(60), true)
                .withOutput("Stereo Out", juce::AudioChannelSet::stereo(), true);
            }
        }
        
        if (hostType.getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone ||
            hostType.getPluginLoadedAs() == AudioProcessor::wrapperType_Unity) {
            return BusesProperties()
                .withInput("Mach1Spatial In", juce::AudioChannelSet::discreteChannels(8), true)
                .withOutput("Stereo Out", juce::AudioChannelSet::stereo(), true);
        }
        
        // STREAMING Monitor instance
        return BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true);
    }
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterChanged(const juce::String &parameterID, float newValue) override;
    std::vector<int> input_channel_indices; // For reordering channel indices based on specific DAW hosts (example: reordering for ProTools 7.1 channel order)
    void fillChannelOrderArray(int numM1InputChannels);
    
#ifndef CUSTOM_CHANNEL_LAYOUT
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
    static juce::String paramMonitorMode;
    static juce::String paramOutputMode;
    
    double processorSampleRate = 44100; // only has to be something for the initilizer to work
    void m1DecodeChangeInputMode(Mach1DecodeAlgoType inputMode);
    MixerSettings monitorSettings;
    juce::PluginHostType hostType;
    bool layoutCreated = false;

    // Orientation Manager/Client
    void setStatus(bool success, std::string message);
    M1OrientationClient m1OrientationClient;
    M1OrientationYPR currentOrientation; // TODO: use Orientation class instead
    M1OrientationYPR previousOrientation;
    // normalize first value
    M1OrientationYPR external_orientation;
    // normalize first value
    M1OrientationYPR previous_external_orientation;

    // Communication to Player and the rest of the M1SpatialSystem
    void timerCallback() override;
    MonitorOSC monitorOSC;
    ScopedPointer<Transport> transport; // TODO: use std::unique_ptr
    
    // TODO: change this when implmenting external mixer
    bool external_spatialmixer_active = false; // global detect spatialmixer
       
	bool isMonitorOSCConnected = false;

    juce::UndoManager mUndoManager;
    juce::AudioProcessorValueTreeState parameters;

private:
    void createLayout();

    std::vector<std::vector<float>> audioDataIn;
    std::vector<float> spatialMixerCoeffs;
    std::vector<std::vector<juce::LinearSmoothedValue<float>>> smoothedChannelCoeffs;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (M1MonitorAudioProcessor)
};
