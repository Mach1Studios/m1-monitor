#pragma once

#include "Config.h"
#include "AlertData.h"
#include "M1Analytics.h"
#include "MonitorOSC.h"
#include "TypesForDataExchange.h"
#include <JuceHeader.h>
#include <Mach1Decode.h>

//==============================================================================
/**
*/

class MonitorOSC; // forward declaration for MonitorOSC
class M1MonitorAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener, public juce::Timer
{
public:
    //==============================================================================
    M1MonitorAudioProcessor();
    ~M1MonitorAudioProcessor() override;

    static AudioProcessor::BusesProperties getHostSpecificLayout()
    {
        // This determines the initial bus i/o for plugin on construction and depends on the `isBusesLayoutSupported()`
        juce::PluginHostType hostType;

        // Multichannel Pro Tools
        // TODO: Check if Ultimate/HD
        if (hostType.isProTools())
        {
            return BusesProperties()
                .withInput("Mach1Spatial Input", juce::AudioChannelSet::create7point1(), true)
                .withOutput("Default Output", juce::AudioChannelSet::stereo(), true);
        }

        // Multichannel DAWs
        if (hostType.isReaper() || hostType.isNuendo() || hostType.isDaVinciResolve() || hostType.isArdour())
        {
            if (hostType.getPluginLoadedAs() == AudioProcessor::wrapperType_VST3)
            {
                return BusesProperties()
                    // VST3 requires named plugin configurations only
                    .withInput("Mach1Spatial In", juce::AudioChannelSet::ambisonic(5), true) // 36 named channel
                    .withOutput("Stereo Out", juce::AudioChannelSet::stereo(), true);
            }
            else
            {
                return BusesProperties()
                    .withInput("Mach1Spatial In", juce::AudioChannelSet::discreteChannels(60), true)
                    .withOutput("Stereo Out", juce::AudioChannelSet::stereo(), true);
            }
        }

        if (hostType.getPluginLoadedAs() == AudioProcessor::wrapperType_Unity)
        {
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
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    std::vector<int> input_channel_indices; // For reordering channel indices based on specific DAW hosts (example: reordering for ProTools 7.1 channel order)
    void fillChannelOrderArray(int numM1InputChannels);

#ifndef CUSTOM_CHANNEL_LAYOUT
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processStereoDownmix(juce::AudioBuffer<float>&);

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter Setup
    juce::AudioProcessorValueTreeState& getValueTreeState();
    static juce::String paramYaw;
    static juce::String paramPitch;
    static juce::String paramRoll;
    static juce::String paramMonitorMode;
    static juce::String paramOutputMode;

    double processorSampleRate = 44100; // only has to be something for the initilizer to work
    void m1DecodeChangeInputMode(Mach1DecodeMode inputMode);
    MixerSettings monitorSettings;
    juce::PluginHostType hostType;
    bool layoutCreated = false;
    bool pluginInitialized = false;

    // Orientation Manager/Client
    void setStatus(bool success, std::string message);
    M1OrientationClient m1OrientationClient;
    Mach1::Float3 m_last_external_ori_degrees;

    // Communication to Player and the rest of the M1SpatialSystem
    void timerCallback() override;
    std::unique_ptr<MonitorOSC> monitorOSC;

    // TODO: change this when implmenting external mixer
    bool external_spatialmixer_active = false; // global detect spatialmixer

    juce::UndoManager mUndoManager;
    juce::AudioProcessorValueTreeState parameters;

    // Analytics for crash reporting
    void addJob(std::function<void()> job);
    void addJob(juce::ThreadPoolJob* job, bool deleteJobWhenFinished);
    void cancelJob(juce::ThreadPoolJob* job);
    juce::ThreadPool& getThreadPool();

    // This will be set by the UI or editor so we can notify it of alerts
    std::function<void(const Mach1::AlertData&)> postAlertToUI;
    void postAlert(const Mach1::AlertData& alert);
    std::vector<Mach1::AlertData> pendingAlerts;

private:
    void updateTransportWithPlayhead();
    void syncParametersWithExternalOrientation();
    void createLayout();

    std::vector<std::vector<float>> audioDataIn;
    std::vector<float> spatialMixerCoeffs;
    std::vector<std::vector<juce::LinearSmoothedValue<float>>> smoothedChannelCoeffs;

    juce::ThreadPool jobThreads{ (std::max)(4, juce::SystemStats::getNumCpus()) };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(M1MonitorAudioProcessor)
};
