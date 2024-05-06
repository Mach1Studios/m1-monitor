/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

/*
 Architecture:

 */

juce::String M1MonitorAudioProcessor::paramYaw("yaw");
juce::String M1MonitorAudioProcessor::paramPitch("pitch");
juce::String M1MonitorAudioProcessor::paramRoll("roll");
juce::String M1MonitorAudioProcessor::paramMonitorMode("monitorMode");
juce::String M1MonitorAudioProcessor::paramOutputMode("outputMode");

//==============================================================================
M1MonitorAudioProcessor::M1MonitorAudioProcessor()
     : AudioProcessor (getHostSpecificLayout()),
    parameters(*this, &mUndoManager, juce::Identifier("M1-Monitor"),
               {
                    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramYaw, 1),
                                                            TRANS("Yaw"),
                                                            juce::NormalisableRange<float>(0.0f, 360.0f, 0.01f), monitorSettings.yaw, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramPitch, 1),
                                                            TRANS("Pitch"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.pitch, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramRoll, 1),
                                                            TRANS("Roll"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.roll, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramMonitorMode, 1), TRANS("Monitor Mode"), 0, 2, monitorSettings.monitor_mode),
                    // Note: Change init output to max bus size when new formats are introduced
                    std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramOutputMode, 1), TRANS("Output Mode"), 0, Mach1DecodeAlgoSpatial_14, Mach1DecodeAlgoSpatial_8),
               })
{
    parameters.addParameterListener(paramYaw, this);
    parameters.addParameterListener(paramPitch, this);
    parameters.addParameterListener(paramRoll, this);
    parameters.addParameterListener(paramMonitorMode, this);
    parameters.addParameterListener(paramOutputMode, this);

    // Setup for Mach1Decode API
    monitorSettings.m1Decode.setPlatformType(Mach1PlatformDefault);
	monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
    monitorSettings.m1Decode.setFilterSpeed(0.99);
    
    smoothedChannelCoeffs.resize(monitorSettings.m1Decode.getFormatChannelCount());
    for (int input_channel = 0; input_channel < monitorSettings.m1Decode.getFormatChannelCount(); input_channel++) {
        smoothedChannelCoeffs[input_channel].resize(2);
    }

    transport = new Transport(&m1OrientationClient);
    
    // We will assume the folders are properly created during the installation step
    juce::File settingsFile;
    // Using common support files installation location
    juce::File m1SupportDirectory = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);

    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
        // test for any mac OS
        settingsFile = m1SupportDirectory.getChildFile("Application Support").getChildFile("Mach1");
    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
        // test for any windows OS
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    } else {
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    }
    settingsFile = settingsFile.getChildFile("settings.json");
    DBG("Opening settings file: " + settingsFile.getFullPathName().quoted());
    
    // Informs OrientationManager that this client is expected to calculate the final orientation and to count instances for error handling
    m1OrientationClient.setClientType("monitor"); // Needs to be set before the init() function
    m1OrientationClient.initFromSettings(settingsFile.getFullPathName().toStdString());
    m1OrientationClient.setStatusCallback(std::bind(&M1MonitorAudioProcessor::setStatus, this, std::placeholders::_1, std::placeholders::_2));
    
    // setup the listener
    monitorOSC.AddListener([&](juce::OSCMessage msg) {
        if (msg.getAddressPattern() == "/YPR-Offset") {
            if (msg.size() >= 1) {
                // Capturing Player's Yaw mouse offset
                if (msg[0].isFloat32()){
                    float yaw = msg[0].getFloat32();
                    // add the offset to the current orientation
                    yaw += parameters.getParameter(paramYaw)->convertTo0to1(monitorSettings.yaw);
                    // manual version of modulo for the endless yaw radial
                    if (yaw < 0.0f) yaw += 1.0f;
                    if (yaw > 1.0f) yaw -= 1.0f;
                    // apply if yaw is active
                    // we negate the right-handed yaw from `currentOrientation` before comparing to the left-handed yaw value
                    if (monitorSettings.yawActive) {
                        currentOrientation.ApplyRotationDegrees({ 0, -((yaw * 360.0f) + monitorSettings.yaw), 0 });
                    } else {
                        currentOrientation.SetRotation({ currentOrientation.GetGlobalRotationAsEulerRadians().GetPitch(), 0, currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
                    }
                    parameters.getParameter(paramYaw)->setValueNotifyingHost(currentOrientation.GetGlobalRotationAsEulerDegrees().Normalized().GetYaw());
                }
                DBG("[OSC] Recieved msg | " + msg.getAddressPattern().toString() + ", Y: "+std::to_string(msg[0].getFloat32()));
            }
            if (msg.size() >= 2) {
                // Capturing Player's Pitch mouse offset
                if (msg[1].isFloat32()){
                    float pitch = msg[1].getFloat32();
                    // apply if pitch is active
                    // we negate the right-handed pitch from `currentOrientation` before comparing to the left-handed pitch value
                    if (monitorSettings.pitchActive) {
                        currentOrientation.ApplyRotationDegrees({ -((pitch * 180.0f) - 90.0f + monitorSettings.pitch), 0, 0 });
                    } else {
                        currentOrientation.SetRotation({ 0, currentOrientation.GetGlobalRotationAsEulerRadians().GetYaw(), currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
                    }
                    parameters.getParameter(paramPitch)->setValueNotifyingHost(currentOrientation.GetGlobalRotationAsEulerDegrees().Normalized().GetPitch());
                }
                DBG("[OSC] Recieved msg | " + msg.getAddressPattern().toString() + ", P: "+std::to_string(msg[1].getFloat32()));
            }
        } else {
            // display a captured unexpected osc message
            if (msg.size() > 0) {
                DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString());
                if (msg[0].isFloat32()) {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + std::to_string(msg[0].getFloat32()));
                } else if (msg[0].isInt32()) {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + std::to_string(msg[0].getInt32()));
                } else if (msg[0].isString()) {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + msg[0].getString());
                }
            }
        }
    });

    // monitorOSC update timer loop (only used for checking the connection)
    startTimer(200);
}

M1MonitorAudioProcessor::~M1MonitorAudioProcessor()
{
    transport = nullptr;
    m1OrientationClient.command_disconnect();
    m1OrientationClient.close();
}

//==============================================================================
const juce::String M1MonitorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool M1MonitorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool M1MonitorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool M1MonitorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double M1MonitorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int M1MonitorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int M1MonitorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void M1MonitorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String M1MonitorAudioProcessor::getProgramName (int index)
{
    return {};
}

void M1MonitorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void M1MonitorAudioProcessor::createLayout(){
    int numInputsFromMonitorSettings = monitorSettings.m1Decode.getFormatChannelCount();
    
    if (external_spatialmixer_active) {
        /// EXTERNAL MULTICHANNEL PROCESSING

        // INPUT
        if (monitorSettings.m1Decode.getFormatChannelCount() == 1){
            getBus(true, 0)->setCurrentLayout(juce::AudioChannelSet::mono());
        }
        else if (monitorSettings.m1Decode.getFormatChannelCount() == 2){
            getBus(true, 0)->setCurrentLayout(juce::AudioChannelSet::stereo());
        }
        // OUTPUT
        getBus(false, 0)->setCurrentLayout(juce::AudioChannelSet::stereo());
    } else {
        /// INTERNAL MULTICHANNEL PROCESSING
        
        // check if there is a mismatch of the current bus size on PT
        if (hostType.isProTools()) {
            // update the monitorSettings if there is a mismatch
            
            // I/O Concept
            // Inputs: for this plugin we allow the m1Decode object to have a higher channel count input mode than what the host allows to support more configurations on channel specific hosts
            // Outputs: this is always stereo as the purpose of this plugin is the decode a stereo output stream via Mach1Decode API

            /// INPUTS
            if (getBus(true, 0)->getCurrentLayout().size() != monitorSettings.m1Decode.getFormatChannelCount()) {
                if (getBus(true, 0)->getCurrentLayout().size() == 4) {
                    monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoType::Mach1DecodeAlgoHorizon_4);
                    m1DecodeChangeInputMode(Mach1DecodeAlgoType::Mach1DecodeAlgoHorizon_4);
                } else if (getBus(true, 0)->getCurrentLayout().size() == 8) {
                    monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_8);
                    m1DecodeChangeInputMode(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_8);
                } else if (getBus(true, 0)->getCurrentLayout().getAmbisonicOrder() == 2) { // if an ambisonic 2nd order
                    monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_8);
                    m1DecodeChangeInputMode(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_8);
                } else if (getBus(true, 0)->getCurrentLayout().getAmbisonicOrder() > 2) { // if an ambisonic bus higher than 2nd order
                    monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_14);
                    m1DecodeChangeInputMode(Mach1DecodeAlgoType::Mach1DecodeAlgoSpatial_14);
                } else {
                    // an unsupported format
                }
            }
        }
    }
    layoutCreated = true; // flow control for static i/o
    updateHostDisplay();
}

//==============================================================================
void M1MonitorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    if (!layoutCreated) {
        createLayout();
    }
    
    // can still be used to calculate coeffs even in STREAMING_PANNER_PLUGIN mode
    processorSampleRate = sampleRate;
    
    if (monitorSettings.m1Decode.getFormatChannelCount() != getMainBusNumInputChannels()){
        bool channel_io_error = -1;
        // error handling here?
    }
    
    // Checks if output bus is non DISCRETE layout and fixes host specific channel ordering issues
    fillChannelOrderArray(monitorSettings.m1Decode.getFormatChannelCount());
    
    // setup initial values for currentOrientation
    Mach1::Float3 new_rot = { monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll }; // in degrees
    currentOrientation.SetRotation(new_rot.EulerRadians());
}

void M1MonitorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void M1MonitorAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    // incoming value expected as normalized
    // use this to update and sync parameter & monitorSettings
    if (parameterID == paramYaw) {
        monitorSettings.yaw = parameters.getParameter(paramYaw)->convertFrom0to1(parameters.getParameter(paramYaw)->getValue());
    } else if (parameterID == paramPitch) {
        monitorSettings.pitch = parameters.getParameter(paramPitch)->convertFrom0to1(parameters.getParameter(paramPitch)->getValue());
    } else if (parameterID == paramRoll) {
        monitorSettings.roll = parameters.getParameter(paramRoll)->convertFrom0to1(parameters.getParameter(paramRoll)->getValue());
    } else if (parameterID == paramMonitorMode) {
        monitorSettings.monitor_mode = (int)parameters.getParameter(paramMonitorMode)->convertFrom0to1(parameters.getParameter(paramMonitorMode)->getValue());
        
        // update gui on other plugins
        if (monitorOSC.IsConnected() && monitorOSC.IsActiveMonitor()) {
            monitorOSC.sendMonitoringMode(monitorSettings.monitor_mode);
        }
    }
    
    // update the gui on other plugins for any changes to YPR
    if (parameterID == paramYaw || parameterID == paramPitch || parameterID == paramRoll) {
        if (monitorOSC.IsConnected() && monitorOSC.IsActiveMonitor()) {
            // update the server and panners of final calculated orientation
            // sending un-normalized full range values in degrees
            monitorOSC.sendMasterYPR(monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll);
        }
    }

    // TODO: add UI for syncing panners to current monitor outputMode and add that outputMode int to this function

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool M1MonitorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::PluginHostType hostType;
    Mach1Decode configTester;
    
    // block plugin if input or output is disabled on construction
    if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
     || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;
    
    if (hostType.isReaper()) {
        return true;
    }
        
    if (hostType.isProTools()) {
        if ((   layouts.getMainInputChannelSet() == juce::AudioChannelSet::quadraphonic()
             || layouts.getMainInputChannelSet() == juce::AudioChannelSet::create7point1()
             || layouts.getMainInputChannelSet() == juce::AudioChannelSet::ambisonic(3)
             || layouts.getMainInputChannelSet() == juce::AudioChannelSet::ambisonic(5)
             || layouts.getMainInputChannelSet() == juce::AudioChannelSet::ambisonic(7)
             )
            &&
            (   layouts.getMainOutputChannelSet().size() == juce::AudioChannelSet::stereo().size() )) {
                return true;
        } else {
            return false;
        }
    } else if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()) {
        // RETURN TRUE FOR EXTERNAL STREAMING MODE
        // hard set {2,2} for streaming use case
        // TODO: Add a check for Reaper + EXTERNAL STREAMING MODE
        return true;
    } else {
        // Test for all available Mach1Encode configs
        // manually maintained for-loop of first enum element to last enum element
        // TODO: brainstorm a way to not require manual maintaining of listed enum elements
        for (int inputEnum = 0; inputEnum != Mach1DecodeAlgoSpatial_14; inputEnum++ ) {
            configTester.setDecodeAlgoType(static_cast<Mach1DecodeAlgoType>(inputEnum));
            // test each input, if the input has the number of channels as the input testing layout has move on to output testing
            if (layouts.getMainInputChannelSet().size() >= configTester.getFormatChannelCount()) {
                // test each output
                if (layouts.getMainOutputChannelSet().size() == juce::AudioChannelSet::stereo().size()){
                    return true;
                }
            }
        }
        return false;
    }
}
#endif

void M1MonitorAudioProcessor::fillChannelOrderArray(int numM1InputChannels) {
    // sets the maximum channels of the current host layout
    juce::AudioChannelSet chanset = getBus(true, 0)->getCurrentLayout();
    int numHostInputChannels = getBus(true, 0)->getNumberOfChannels();
    
    // sets the maximum channels of the current selected m1 input layout
    std::vector<juce::AudioChannelSet::ChannelType> chan_types;
    chan_types.resize(numM1InputChannels);
    input_channel_indices.resize(numM1InputChannels);

    if(!chanset.isDiscreteLayout()) { // Check for DAW specific instructions
        if (hostType.isProTools() && chanset.size() == 8 && chanset.getDescription().contains(juce::String("7.1 Surround"))){
            // TODO: Remove this and figure out why we cannot use what is in "else" on PT 7.1
            chan_types[0] = juce::AudioChannelSet::ChannelType::left;
            chan_types[1] = juce::AudioChannelSet::ChannelType::centre;
            chan_types[2] = juce::AudioChannelSet::ChannelType::right;
            chan_types[3] = juce::AudioChannelSet::ChannelType::leftSurroundSide;
            chan_types[4] = juce::AudioChannelSet::ChannelType::rightSurroundSide;
            chan_types[5] = juce::AudioChannelSet::ChannelType::leftSurroundRear;
            chan_types[6] = juce::AudioChannelSet::ChannelType::rightSurroundRear;
            chan_types[7] = juce::AudioChannelSet::ChannelType::LFE;
        } else {
            // Get the index of each channel supplied via JUCE
            for (int i = 0; i < numM1InputChannels; i ++) {
                chan_types[i] = chanset.getTypeOfChannel(i);
            }
        }
        // Apply the index
        for (int i = 0; i < numM1InputChannels; i ++) {
            input_channel_indices[i] = chanset.getChannelIndexForType(chan_types[i]);
        }
    } else { // is a discrete channel layout
        for (int i = 0; i < numM1InputChannels; ++i){
            input_channel_indices[i] = i;
        }
    }
}

void M1MonitorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numInputChannels  = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();
    
    // if you've got more output channels than input clears extra outputs
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
    
    // transport
    updateTransportWithPlayhead();

    // m1_orientation_client update
    if (m1OrientationClient.isConnectedToServer()) {

        monitorOSC.sendMonitoringMode(monitorSettings.monitor_mode);
        monitorOSC.sendMasterYPR(monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll);

        // update the external orientation normalised
        external_orientation = m1OrientationClient.getOrientation();
        
        // retrieve normalized values and add the current external device orientation
        if (previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetYaw() != external_orientation.GetGlobalRotationAsEulerDegrees().GetYaw()) {
            
            if (monitorSettings.yawActive) {
                currentOrientation.ApplyRotationDegrees({ 0, external_orientation.GetGlobalRotationAsEulerDegrees().GetYaw() - previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetYaw() + -monitorSettings.yaw, 0 });
            } else {
                currentOrientation.SetRotation({ currentOrientation.GetGlobalRotationAsEulerRadians().GetPitch(), 0, currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
            }
            parameters.getParameter(paramYaw)->setValueNotifyingHost(currentOrientation.GetGlobalRotationAsEulerDegrees().Normalized().GetYaw());
        } else {
            // since there is no change from external sources we will just update the orientation via monitorSettings
            currentOrientation.ApplyRotationDegrees({ 0, -monitorSettings.yaw, 0 });
        }
        
        if (previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetPitch() != external_orientation.GetGlobalRotationAsEulerDegrees().GetPitch()) {
            
            if (monitorSettings.pitchActive) {
                currentOrientation.ApplyRotationDegrees({ external_orientation.GetGlobalRotationAsEulerDegrees().GetPitch() - previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetPitch() + -monitorSettings.pitch, 0, 0 });
            } else {
                currentOrientation.SetRotation({ 0.5, currentOrientation.GetGlobalRotationAsEulerRadians().GetYaw(), currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
            }
            parameters.getParameter(paramPitch)->setValueNotifyingHost(currentOrientation.GetGlobalRotationAsEulerDegrees().Normalized().GetPitch());
        } else {
            // since there is no change from external sources we will just update the orientation via monitorSettings
            currentOrientation.ApplyRotationDegrees({ -monitorSettings.pitch, 0, 0 });
        }
        
        if (previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetRoll() != external_orientation.GetGlobalRotationAsEulerDegrees().GetRoll()) {
            if (monitorSettings.rollActive) {
                currentOrientation.ApplyRotationDegrees({ 0, 0, external_orientation.GetGlobalRotationAsEulerDegrees().GetRoll() - previous_external_orientation.GetGlobalRotationAsEulerDegrees().GetRoll() + monitorSettings.roll });
            } else {
                currentOrientation.SetRotation({ currentOrientation.GetGlobalRotationAsEulerRadians().GetPitch(), currentOrientation.GetGlobalRotationAsEulerRadians().GetYaw(), 0.5 });
            }
            parameters.getParameter(paramPitch)->setValueNotifyingHost(currentOrientation.GetGlobalRotationAsEulerDegrees().Normalized().GetPitch());
        } else {
            // since there is no change from external sources we will just update the orientation via monitorSettings
            currentOrientation.ApplyRotationDegrees({ 0, 0, monitorSettings.roll });
        }
                
        // store orientation for next loop's delta check
        previous_external_orientation = external_orientation;
        
    } else {
        // update orientation without external orientation
        if (monitorSettings.yawActive) {
            currentOrientation.ApplyRotationDegrees({ 0, parameters.getParameter(paramYaw)->getValue() * 360.0f, 0 });
        } else {
            currentOrientation.SetRotation({ currentOrientation.GetGlobalRotationAsEulerRadians().GetPitch(), 0, currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
        }

        if (monitorSettings.pitchActive) {
            currentOrientation.ApplyRotationDegrees({ (parameters.getParameter(paramPitch)->getValue() * 180.0f) - 90.0f, 0, 0 });
        } else {
            currentOrientation.SetRotation({ 0.5, currentOrientation.GetGlobalRotationAsEulerRadians().GetYaw(), currentOrientation.GetGlobalRotationAsEulerRadians().GetRoll()});
        }
        
        if (monitorSettings.rollActive) {
            currentOrientation.ApplyRotationDegrees({ 0, 0, (parameters.getParameter(paramRoll)->getValue() * 180.0f) - 90.0f });
        } else {
            currentOrientation.SetRotation({ currentOrientation.GetGlobalRotationAsEulerRadians().GetPitch(), currentOrientation.GetGlobalRotationAsEulerRadians().GetYaw(), 0.5 });
        }
    }

    // Mach1Decode processing loop
    monitorSettings.m1Decode.setRotationDegrees({ -currentOrientation.GetGlobalRotationAsEulerDegrees().GetYaw(), -currentOrientation.GetGlobalRotationAsEulerDegrees().GetPitch(), currentOrientation.GetGlobalRotationAsEulerDegrees().GetRoll() });
    monitorSettings.m1Decode.beginBuffer();
    spatialMixerCoeffs = monitorSettings.m1Decode.decodeCoeffs();
    monitorSettings.m1Decode.endBuffer();
    
    // Update spatial mixer coeffs from Mach1Decode for a smoothed value
    for (int channel = 0; channel < monitorSettings.m1Decode.getFormatChannelCount(); ++channel) {
        smoothedChannelCoeffs[channel][0].setTargetValue(spatialMixerCoeffs[channel * 2    ]); // Left output coeffs
        smoothedChannelCoeffs[channel][1].setTargetValue(spatialMixerCoeffs[channel * 2 + 1]); // Right output coeffs
    }
    
    juce::AudioBuffer<float> tempBuffer((std::max)(buffer.getNumChannels()*2, monitorSettings.m1Decode.getFormatCoeffCount()), buffer.getNumSamples());
    tempBuffer.clear();
    
    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);

    if (getMainBusNumInputChannels() >= monitorSettings.m1Decode.getFormatChannelCount()){
        // TODO: Setup an else case for streaming input or error message
  
        // Internally downmix a stereo output stream and end the processBlock()
        if (monitorSettings.monitor_mode == 2) {
            processStereoDownmix(buffer);
            return;
        }
        
        // Setup buffers for Left & Right outputs
        for (auto channel = 0; channel < numInputChannels; ++channel){
            tempBuffer.copyFrom(channel * 2    , 0, buffer, channel, 0, buffer.getNumSamples());
            tempBuffer.copyFrom(channel * 2 + 1, 0, buffer, channel, 0, buffer.getNumSamples());
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int input_channel = 0; input_channel < buffer.getNumChannels(); input_channel++) {
                buffer.getWritePointer(input_channel)[sample] = 0;
            }
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int input_channel = 0; input_channel < monitorSettings.m1Decode.getFormatChannelCount(); input_channel++) {
                // We apply a channel re-ordering for DAW canonical specific Input channel configrations via fillChannelOrder() and `input_channel_reordered`
                // Input channel reordering from fillChannelOrder()
                int input_channel_reordered = input_channel_indices[input_channel];
                
                outBufferL[sample] += tempBuffer.getReadPointer(input_channel * 2    )[sample] * smoothedChannelCoeffs[input_channel_reordered][0].getNextValue();
                outBufferR[sample] += tempBuffer.getReadPointer(input_channel * 2 + 1)[sample] * smoothedChannelCoeffs[input_channel_reordered][1].getNextValue();
            }
        }
    } else {
//        // Invalid Decode I/O; clear buffers
//        // TODO: Prepare for input streamed case when needed here
//        for (int channel = getTotalNumInputChannels(); channel <= getTotalNumOutputChannels(); ++channel)
//            buffer.clear(channel, 0, buffer.getNumSamples());
    }
    
    // update orientation for calculating offsets
    previousOrientation = currentOrientation;
    
    // clear remaining input channels
    for (auto channel = 2; channel < getTotalNumInputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

//==============================================================================
void M1MonitorAudioProcessor::processStereoDownmix(juce::AudioBuffer<float>& buffer)
{
    auto numInputChannels  = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();

    juce::AudioBuffer<float> tempBuffer(numInputChannels*2 + 2, buffer.getNumSamples());
    tempBuffer.clear();
    
    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);
    
    // Stereo Downmix Logic
    for (auto i = 0; i < numInputChannels; ++i){
        tempBuffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    }
    
    if (numInputChannels == 4 || numInputChannels == 8) {
        for (int channel = 0; channel < numInputChannels; channel++) {
            for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
                outBufferL[sample] += tempBuffer.getReadPointer(channel * 2    )[sample] / std::sqrt(2)/(numInputChannels/2);
                outBufferR[sample] += tempBuffer.getReadPointer(channel * 2 + 1)[sample] / std::sqrt(2)/(numInputChannels/2);
            }
        }
    }
    else if (numInputChannels == 12){
        //INDEX:          0,1,2,3,4,5, 6
        int mixMapL[] = { 0,2,4,6,8,10,11 };
        int mixMapR[] = { 1,3,5,7,8,9, 10 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (tempBuffer.getReadPointer(mixMapL[0])[i]
                + tempBuffer.getReadPointer(mixMapL[1])[i]
                + tempBuffer.getReadPointer(mixMapL[2])[i]
                + tempBuffer.getReadPointer(mixMapL[3])[i]
                + tempBuffer.getReadPointer(mixMapL[4])[i]/2
                + tempBuffer.getReadPointer(mixMapL[5])[i]/2
                + tempBuffer.getReadPointer(mixMapL[6])[i])/std::sqrt(2)/(numInputChannels/2);

            outBufferR[i] = (tempBuffer.getReadPointer(mixMapR[0])[i]
                + tempBuffer.getReadPointer(mixMapL[1])[i]
                + tempBuffer.getReadPointer(mixMapL[2])[i]
                + tempBuffer.getReadPointer(mixMapL[3])[i]
                + tempBuffer.getReadPointer(mixMapL[4])[i]/2
                + tempBuffer.getReadPointer(mixMapL[5])[i]
                + tempBuffer.getReadPointer(mixMapL[6])[i]/2)/std::sqrt(2)/(numInputChannels/2);
        }
    } else if (numInputChannels == 14){
        //INDEX:          0,1,2,3,4,5, 6 ,7 ,8
        int mixMapL[] = { 0,2,4,6,8,10,11,12,13 };
        int mixMapR[] = { 1,3,5,7,8,9, 10,12,13 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (tempBuffer.getReadPointer(mixMapL[0])[i]
                + tempBuffer.getReadPointer(mixMapL[1])[i]
                + tempBuffer.getReadPointer(mixMapL[2])[i]
                + tempBuffer.getReadPointer(mixMapL[3])[i]
                + tempBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[5])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[6])[i]
                + tempBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2)/std::sqrt(2)/(numInputChannels/2);

            outBufferR[i] = (tempBuffer.getReadPointer(mixMapR[0])[i]
                + tempBuffer.getReadPointer(mixMapL[1])[i]
                + tempBuffer.getReadPointer(mixMapL[2])[i]
                + tempBuffer.getReadPointer(mixMapL[3])[i]
                + tempBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[5])[i]
                + tempBuffer.getReadPointer(mixMapL[6])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + tempBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2)/std::sqrt(2)/(numInputChannels/2);
        }
    }
}

//==============================================================================
void M1MonitorAudioProcessor::timerCallback() {
    // Added if we need to move the OSC stuff from the processorblock
    monitorOSC.update(); // test for connection
}

//==============================================================================
void M1MonitorAudioProcessor::setStatus(bool success, std::string message)
{
    std::cout << success << " , " << message << std::endl;
}

//==============================================================================
bool M1MonitorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* M1MonitorAudioProcessor::createEditor()
{
    return new M1MonitorAudioProcessorEditor (*this);
}

//==============================================================================
void M1MonitorAudioProcessor::m1DecodeChangeInputMode(Mach1DecodeAlgoType decodeMode)
{
    monitorSettings.m1Decode.setDecodeAlgoType(decodeMode);
    auto inputChannelsCount = monitorSettings.m1Decode.getFormatChannelCount();
    smoothedChannelCoeffs.resize(inputChannelsCount);

    // Checks if input bus is non DISCRETE layout and fixes host specific channel ordering issues
    fillChannelOrderArray(monitorSettings.m1Decode.getFormatChannelCount());

    for (int input_channel = 0; input_channel < inputChannelsCount; input_channel++) {
        smoothedChannelCoeffs[input_channel].resize(2);
        for (int output_channel = 0; output_channel < 2; output_channel++) {
            smoothedChannelCoeffs[input_channel][output_channel].reset(processorSampleRate, (double)0.01);
        }
    }
}

//==============================================================================
void M1MonitorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream stream(destData, false);
    parameters.state.writeToStream(stream);
}

void M1MonitorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        parameters.state = tree;
    }
}

void M1MonitorAudioProcessor::updateTransportWithPlayhead() {
    AudioPlayHead* ph = getPlayHead();

    if (ph == nullptr)
    {
        return;
    }

    auto play_head_position = ph->getPosition();

    if (!play_head_position.hasValue()) {
        return;
    }

    if (play_head_position->getTimeInSeconds().hasValue()) {
        transport->setTimeInSeconds(*play_head_position->getTimeInSeconds());
    }

    transport->setIsPlaying(play_head_position->getIsPlaying());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new M1MonitorAudioProcessor();
}
