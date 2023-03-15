/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::String M1MonitorAudioProcessor::paramYaw("yaw");
juce::String M1MonitorAudioProcessor::paramPitch("pitch");
juce::String M1MonitorAudioProcessor::paramRoll("roll");
juce::String M1MonitorAudioProcessor::paramYawEnable("enableYaw");
juce::String M1MonitorAudioProcessor::paramPitchEnable("enablePitch");
juce::String M1MonitorAudioProcessor::paramRollEnable("enableRoll");
juce::String M1MonitorAudioProcessor::paramMonitorMode("monitorMode");
juce::String M1MonitorAudioProcessor::paramDecodeMode("decodeMode");

//==============================================================================
M1MonitorAudioProcessor::M1MonitorAudioProcessor()
     : AudioProcessor (getHostSpecificLayout()),
    parameters(*this, &mUndoManager, juce::Identifier("M1-Monitor"),
               {
                    std::make_unique<juce::AudioParameterFloat>(paramYaw,
                                                            TRANS("Yaw"),
                                                            juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f), monitorSettings.yaw, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(paramPitch,
                                                            TRANS("Pitch"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.pitch, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(paramRoll,
                                                            TRANS("Roll"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.roll, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterBool>(paramYawEnable, TRANS("Enable Yaw"), monitorSettings.yawActive),
                    std::make_unique<juce::AudioParameterBool>(paramPitchEnable, TRANS("Enable Pitch"), monitorSettings.pitchActive),
                    std::make_unique<juce::AudioParameterBool>(paramRollEnable, TRANS("Enable Roll"), monitorSettings.rollActive),
                    std::make_unique<juce::AudioParameterInt>(paramMonitorMode, TRANS("Monitor Mode"), 0, 3, monitorSettings.monitor_mode),
               })
{
    parameters.addParameterListener(paramYaw, this);
    parameters.addParameterListener(paramPitch, this);
    parameters.addParameterListener(paramRoll, this);
    parameters.addParameterListener(paramYawEnable, this);
    parameters.addParameterListener(paramPitchEnable, this);
    parameters.addParameterListener(paramRollEnable, this);
    parameters.addParameterListener(paramMonitorMode, this);
    
    // Setup for Mach1Decode API
    monitorSettings.m1Decode.setPlatformType(Mach1PlatformDefault);
    monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
    monitorSettings.m1Decode.setFilterSpeed(0.99);
    
    // TODO: ensure we call the changedecode function when we introduce a dropdown menu changer
    smoothedChannelCoeffs.resize(monitorSettings.m1Decode.getFormatChannelCount());
    for (int input_channel = 0; input_channel < monitorSettings.m1Decode.getFormatChannelCount(); input_channel++) {
        smoothedChannelCoeffs[input_channel].resize(monitorSettings.m1Decode.getFormatChannelCount());
    }

    transport = new Transport();
    transport->setProcessor(this);
    transport->setOrientationClient(&m1OrientationOSCClient);
}

M1MonitorAudioProcessor::~M1MonitorAudioProcessor()
{
    transport = nullptr;
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
    int numInChans = monitorSettings.m1Decode.getFormatChannelCount();
    
    if (external_spatialmixer_active) {
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
        // internal processing
        
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
}

void M1MonitorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void M1MonitorAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    if (parameterID == paramYaw) {
        parameters.getParameter(paramYaw)->setValue(newValue);
    } else if (parameterID == paramPitch) {
        parameters.getParameter(paramPitch)->setValue(newValue);
    } else if (parameterID == paramRoll) {
        parameters.getParameter(paramRoll)->setValue(newValue);
    } else if (parameterID == paramYawEnable) {
        parameters.getParameter(paramYawEnable)->setValue(newValue);
    } else if (parameterID == paramPitchEnable) {
        parameters.getParameter(paramPitchEnable)->setValue(newValue);
    } else if (parameterID == paramRollEnable) {
        parameters.getParameter(paramRollEnable)->setValue(newValue);
    } else if (parameterID == paramMonitorMode) {
        parameters.getParameter(paramMonitorMode)->setValue(newValue);
    }
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
        if ((   layouts.getMainInputChannelSet() == juce::AudioChannelSet::create7point1()
             || layouts.getMainInputChannelSet() == juce::AudioChannelSet::quadraphonic())
            &&
            (   layouts.getMainOutputChannelSet().size() == juce::AudioChannelSet::stereo().size() )) {
                return true;
        } else {
            return false;
        }
    } else if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()) {
        // RETURN TRUE FOR EXTERNAL STREAMING MODE
        // hard set {2,2} for streaming use case
        return true;
    } else {
        // Test for all available Mach1Encode configs
        // manually maintained for-loop of first enum element to last enum element
        // TODO: brainstorm a way to not require manual maintaining of listed enum elements
        for (int inputEnum = 0; inputEnum != Mach1DecodeAlgoSpatial_60; inputEnum++ ) {
            configTester.setDecodeAlgoType(static_cast<Mach1DecodeAlgoType>(inputEnum));
            // test each input, if the input has the number of channels as the input testing layout has move on to output testing
            if (layouts.getMainInputChannelSet().size() == configTester.getFormatChannelCount()) {
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

void M1MonitorAudioProcessor::fillChannelOrderArray(int numInputChannels) {
    orderOfChans.resize(numInputChannels);
    input_channel_indices.resize(numInputChannels);
    
    juce::AudioChannelSet chanset = getBus(true, 0)->getCurrentLayout();
    
    if(!chanset.isDiscreteLayout() && numInputChannels == 8) {
        // Layout for Pro Tools
        if (hostType.isProTools()){
            orderOfChans[0] = juce::AudioChannelSet::ChannelType::left;
            orderOfChans[1] = juce::AudioChannelSet::ChannelType::centre;
            orderOfChans[2] = juce::AudioChannelSet::ChannelType::right;
            orderOfChans[3] = juce::AudioChannelSet::ChannelType::leftSurroundSide;
            orderOfChans[4] = juce::AudioChannelSet::ChannelType::rightSurroundSide;
            orderOfChans[5] = juce::AudioChannelSet::ChannelType::leftSurroundRear;
            orderOfChans[6] = juce::AudioChannelSet::ChannelType::rightSurroundRear;
            orderOfChans[7] = juce::AudioChannelSet::ChannelType::LFE;
        } else {
            orderOfChans[0] = juce::AudioChannelSet::ChannelType::left;
            orderOfChans[1] = juce::AudioChannelSet::ChannelType::right;
            orderOfChans[2] = juce::AudioChannelSet::ChannelType::centre;
            orderOfChans[3] = juce::AudioChannelSet::ChannelType::LFE;
            orderOfChans[4] = juce::AudioChannelSet::ChannelType::leftSurroundSide;
            orderOfChans[5] = juce::AudioChannelSet::ChannelType::rightSurroundSide;
            orderOfChans[6] = juce::AudioChannelSet::ChannelType::leftSurroundRear;
            orderOfChans[7] = juce::AudioChannelSet::ChannelType::rightSurroundRear;
        }
        if (chanset.size() >= 8) {
            for (int i = 0; i < numInputChannels; i ++) {
                input_channel_indices[i] = chanset.getChannelIndexForType(orderOfChans[i]);
            }
        }
    } else if (!chanset.isDiscreteLayout() && numInputChannels == 4){
        // Layout for Pro Tools
        if (hostType.isProTools()) {
            orderOfChans[0] = juce::AudioChannelSet::ChannelType::left;
            orderOfChans[1] = juce::AudioChannelSet::ChannelType::right;
            orderOfChans[2] = juce::AudioChannelSet::ChannelType::rightSurround;
            orderOfChans[3] = juce::AudioChannelSet::ChannelType::leftSurround;
        } else {
            orderOfChans[0] = juce::AudioChannelSet::ChannelType::left;
            orderOfChans[1] = juce::AudioChannelSet::ChannelType::right;
            orderOfChans[2] = juce::AudioChannelSet::ChannelType::leftSurround;
            orderOfChans[3] = juce::AudioChannelSet::ChannelType::rightSurround;
        }
        if (chanset.size() >= 4) {
            for (int i = 0; i < numInputChannels; i ++) {
                input_channel_indices[i] = chanset.getChannelIndexForType(orderOfChans[i]);
            }
        }
    } else {
        for (int i = 0; i < numInputChannels; ++i){
            orderOfChans[i] = juce::AudioChannelSet::ChannelType::discreteChannel0;
            input_channel_indices[i] = i;
        }
    }
}

void M1MonitorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // transport
    transport->update();
    
    juce::ScopedNoDenormals noDenormals;
    auto numInputChannels  = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();

    if (numInputChannels == 4){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoHorizon_4);
    } else if (numInputChannels == 8){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
    } else if (numInputChannels == 12){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_12);
    } else if (numInputChannels == 14){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_14);
    } else if (numInputChannels == 32){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_32);
    } else if (numInputChannels == 36){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_36);
    } else if (numInputChannels == 48){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_48);
    } else if (numInputChannels == 60){
        monitorSettings.m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_60);
    }
    
    // if you've got more output channels than input clears extra outputs
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
    
    // Mach1Decode processing loop
    Mach1Point3D currentOrientation;
    // retrieve normalized values
    (parameters.getParameter(paramYawEnable)->getValue()) ? currentOrientation.x = parameters.getParameter(paramYaw)->getValue() : currentOrientation.x = 0.0f;
    (parameters.getParameter(paramPitchEnable)->getValue()) ? currentOrientation.y = parameters.getParameter(paramPitch)->getValue() : currentOrientation.y = 0.0f;
    (parameters.getParameter(paramRollEnable)->getValue()) ? currentOrientation.z = parameters.getParameter(paramRoll)->getValue() : currentOrientation.z = 0.0f;
    monitorSettings.m1Decode.setRotation(currentOrientation);
    monitorSettings.m1Decode.beginBuffer();
    spatialMixerCoeffs = monitorSettings.m1Decode.decodeCoeffs();
    monitorSettings.m1Decode.endBuffer();
    
    // Update spatial mixer coeffs from Mach1Decode for a smoothed value
    for (int channel = 0; channel < monitorSettings.m1Decode.getFormatChannelCount(); ++channel) {
        smoothedChannelCoeffs[channel][0].setTargetValue(spatialMixerCoeffs[channel * 2    ]); // Left output coeffs
        smoothedChannelCoeffs[channel][1].setTargetValue(spatialMixerCoeffs[channel * 2 + 1]); // Right output coeffs
    }
    
    juce::AudioBuffer<float> tempBuffer(monitorSettings.m1Decode.getFormatCoeffCount(), buffer.getNumSamples());
    tempBuffer.clear();
    
    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);

    if (getMainBusNumInputChannels() == monitorSettings.m1Decode.getFormatChannelCount()){
        // TODO: Setup an else case for streaming input or error message
  
//        TODO: Setup monitor modes
//        if (stereoDownmix) {
//            processStereoDownmix(buffer);
//            return;
//        }
        
        // Setup buffers for Left & Right outputs, correct for PT 7.1 buss
        if (getMainBusNumInputChannels() == 8 && hostType.isProTools()){
            AudioChannelSet inputChannelSet = getBus(true, 0)->getCurrentLayout();
            tempBuffer.copyFrom(0, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::left), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(1, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::left), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(2, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::centre), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(3, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::centre), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(4, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::right), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(5, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::right), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(6, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundSide), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(7, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundSide), 0, buffer.getNumSamples());
            
            tempBuffer.copyFrom(8, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundSide), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(9, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundSide), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(10, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundRear), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(11, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundRear), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(12, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundRear), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(13, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundRear), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(14, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::LFE), 0, buffer.getNumSamples());
            tempBuffer.copyFrom(15, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::LFE), 0, buffer.getNumSamples());
        } else {
            for (auto channel = 0; channel < numInputChannels; ++channel){
                tempBuffer.copyFrom(channel * 2    , 0, buffer, channel, 0, buffer.getNumSamples());
                tempBuffer.copyFrom(channel * 2 + 1, 0, buffer, channel, 0, buffer.getNumSamples());
            }
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int input_channel = 0; input_channel < buffer.getNumChannels(); input_channel++) {
                buffer.getWritePointer(input_channel)[sample] = 0;
            }
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int input_channel = 0; input_channel < buffer.getNumChannels(); input_channel++) {
                outBufferL[sample] += tempBuffer.getReadPointer(input_channel * 2    )[sample] * smoothedChannelCoeffs[input_channel][0].getNextValue();
                outBufferR[sample] += tempBuffer.getReadPointer(input_channel * 2 + 1)[sample] * smoothedChannelCoeffs[input_channel][1].getNextValue();
            }
        }
    } else {
        // Invalid Decode I/O; clear buffers
        // TODO: Prepare for input streamed case when needed here
        for (int channel = getTotalNumInputChannels(); channel <= getTotalNumOutputChannels(); ++channel)
            buffer.clear(channel, 0, buffer.getNumSamples());
    }
    
    // clear remaining input channels
    for (auto channel = 2; channel < getTotalNumInputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

//==============================================================================
void M1MonitorAudioProcessor::processStereoDownmix(juce::AudioBuffer<float>& buffer)
{
    auto numInputChannels  = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();

    juce::AudioBuffer<float> tempBuffer(monitorSettings.m1Decode.getFormatCoeffCount(), buffer.getNumSamples());
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
                outBufferL[sample] =+ tempBuffer.getReadPointer(channel * 2    )[sample] / std::sqrt(2)/(numInputChannels/2);
                outBufferR[sample] =+ tempBuffer.getReadPointer(channel * 2 + 1)[sample] / std::sqrt(2)/(numInputChannels/2);
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
    // TODO: implement stereo downmix for 32,36,48,60
    }
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
void M1MonitorAudioProcessor::m1DecodeChangeInputMode(Mach1DecodeAlgoType inputMode) {

    monitorSettings.m1Decode.setDecodeAlgoType(inputMode);
    auto inputChannelsCount = monitorSettings.m1Decode.getFormatChannelCount();
    smoothedChannelCoeffs.resize(inputChannelsCount);

    // Checks if input bus is non DISCRETE layout and fixes host specific channel ordering issues
    fillChannelOrderArray(inputChannelsCount);

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

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new M1MonitorAudioProcessor();
}
