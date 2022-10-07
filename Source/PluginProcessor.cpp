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
     : AudioProcessor (BusesProperties()
//                       #if (JucePlugin_Build_AAX || JucePlugin_Build_RTAS)
//                       .withInput("Default Output", juce::AudioChannelSet::create7point1(), true)
//                       #else
                       .withInput ("Mach1 Output 1", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 2", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 3", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 4", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 5", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 6", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 7", juce::AudioChannelSet::mono(), true)
                       .withInput ("Mach1 Output 8", juce::AudioChannelSet::mono(), true)
//                       #endif
                       .withOutput("Stereo Output", juce::AudioChannelSet::stereo(), true)
                       ),
    parameters(*this, &mUndoManager, juce::Identifier("M1-Monitor"),
               {
                    std::make_unique<juce::AudioParameterFloat>(paramYaw,
                                                            TRANS("Yaw"),
                                                            juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f), 0.0f, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(paramPitch,
                                                            TRANS("Pitch"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), 0.0f, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterFloat>(paramRoll,
                                                            TRANS("Roll"),
                                                            juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), 0.0f, "", juce::AudioProcessorParameter::genericParameter,
                                                            [](float v, int) { return juce::String (v, 1) + "°"; },
                                                            [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                    std::make_unique<juce::AudioParameterBool>(paramYawEnable, TRANS("Enable Yaw"), true),
                    std::make_unique<juce::AudioParameterBool>(paramPitchEnable, TRANS("Enable Pitch"), true),
                    std::make_unique<juce::AudioParameterBool>(paramRollEnable, TRANS("Enable Roll"), true),
                    std::make_unique<juce::AudioParameterInt>(paramMonitorMode, TRANS("Monitor Mode"), 0, 3, 0),
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
    m1Decode.setPlatformType(Mach1PlatformDefault);
    m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
    m1Decode.setFilterSpeed(0.99);
}

M1MonitorAudioProcessor::~M1MonitorAudioProcessor()
{
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
void M1MonitorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    smoothedChannelCoeffs.resize(m1Decode.getFormatCoeffCount());
    spatialMixerCoeffs.resize(m1Decode.getFormatCoeffCount());
    for (int input_channel = 0; input_channel < m1Decode.getFormatChannelCount(); input_channel++) {
        smoothedChannelCoeffs[input_channel * 2].reset(sampleRate, (double)0.01);
        smoothedChannelCoeffs[input_channel * 2 + 1].reset(sampleRate, (double)0.01);
    }
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
    Mach1Decode configTester;
    
    // block plugin if input or output is disabled on construction
    if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
     || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;
    
    // manually maintained for-loop of first enum element to last enum element
    // TODO: brainstorm a way to not require manual maintaining of listed enum elements
    for (int inputEnum = Mach1DecodeAlgoSpatial_8; inputEnum != Mach1DecodeAlgoSpatial_32; inputEnum++ ) {
        configTester.setDecodeAlgoType(static_cast<Mach1DecodeAlgoType>(inputEnum));
        // test each input, if the input has the number of channels as the input testing layout has move on to output testing
        if (layouts.getMainInputChannels() == configTester.getFormatChannelCount()) {
            // test each output
            if (layouts.getMainOutputChannels() == 2){
                return true;
            }
        }
    }
    return false;
}
#endif

void M1MonitorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (totalNumInputChannels == 4){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoHorizon_4);
    } else if (totalNumInputChannels == 8){
        bool useIsotropic = true; // TODO: implement this switch
        if (useIsotropic) {
            m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
        } else {
            m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatialAlt_8);
        }
    } else if (totalNumInputChannels == 12){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_12);
    } else if (totalNumInputChannels == 14){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_14);
    } else if (totalNumInputChannels == 16){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_16);
    } else if (totalNumInputChannels == 18){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_18);
    } else if (totalNumInputChannels == 32){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_32);
    } else if (totalNumInputChannels == 36){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_36);
    } else if (totalNumInputChannels == 48){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_48);
    } else if (totalNumInputChannels == 60){
        m1Decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_60);
    }
    
    // if you've got more output channels than input clears extra outputs
    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
    
    // Mach1Decode processing loop
    Mach1Point3D currentOrientation;
    // retrieve normalized values
    (parameters.getParameter(paramYawEnable)->getValue()) ? currentOrientation.x = parameters.getParameter(paramYaw)->getValue() : currentOrientation.x = 0.0f;
    (parameters.getParameter(paramPitchEnable)->getValue()) ? currentOrientation.y = parameters.getParameter(paramPitch)->getValue() : currentOrientation.y = 0.0f;
    (parameters.getParameter(paramRollEnable)->getValue()) ? currentOrientation.z = parameters.getParameter(paramRoll)->getValue() : currentOrientation.z = 0.0f;
    m1Decode.setRotation(currentOrientation);
    m1Decode.beginBuffer();
    spatialMixerCoeffs = m1Decode.decodeCoeffs();
    m1Decode.endBuffer();
    
    // Update spatial mixer coeffs from Mach1Decode for a smoothed value
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        smoothedChannelCoeffs[channel * 2    ].setTargetValue(spatialMixerCoeffs[channel * 2    ]);
        smoothedChannelCoeffs[channel * 2 + 1].setTargetValue(spatialMixerCoeffs[channel * 2 + 1]);
    }
    
    scratchBuffer.setSize(totalNumInputChannels * 2, buffer.getNumSamples());
    scratchBuffer.clear();
    
    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);
    std::vector<float> spatialCoeffsBufferL, spatialCoeffsBufferR;

    if (totalNumInputChannels == m1Decode.getFormatChannelCount()){ // dumb safety check, TODO: do better i/o error handling
  
//        TODO: Setup monitor modes
//        if (stereoDownmix) {
//            processStereoDownmix(buffer);
//            return;
//        }
        
        // Setup buffers for Left & Right outputs, correct for PT 7.1 buss
        if (totalNumInputChannels == 8 && hostType.isProTools()){
            AudioChannelSet inputChannelSet = getBus(true, 0)->getCurrentLayout();
            scratchBuffer.copyFrom(0, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::left), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(1, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::left), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(2, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::centre), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(3, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::centre), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(4, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::right), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(5, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::right), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(6, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundSide), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(7, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundSide), 0, buffer.getNumSamples());
            
            scratchBuffer.copyFrom(8, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundSide), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(9, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundSide), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(10, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundRear), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(11, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::leftSurroundRear), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(12, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundRear), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(13, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::rightSurroundRear), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(14, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::LFE), 0, buffer.getNumSamples());
            scratchBuffer.copyFrom(15, 0, buffer, inputChannelSet.getChannelIndexForType(AudioChannelSet::ChannelType::LFE), 0, buffer.getNumSamples());
        } else {
            for (auto channel = 0; channel < totalNumInputChannels; ++channel){
                scratchBuffer.copyFrom(channel * 2    , 0, buffer, channel, 0, buffer.getNumSamples());
                scratchBuffer.copyFrom(channel * 2 + 1, 0, buffer, channel, 0, buffer.getNumSamples());
            }
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
                buffer.getWritePointer(channel)[sample] = 0;
            }
        }
        
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            for (int channel = 0; channel < totalNumInputChannels; channel++) {
                outBufferL[sample] += scratchBuffer.getReadPointer(channel * 2    )[sample] * smoothedChannelCoeffs[channel * 2    ].getNextValue();
                outBufferR[sample] += scratchBuffer.getReadPointer(channel * 2 + 1)[sample] * smoothedChannelCoeffs[channel * 2 + 1].getNextValue();
            }
        }
    } else {
        // Invalid Decode I/O; clear buffers
        for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
            buffer.clear(channel, 0, buffer.getNumSamples());
    }
    
    // clear remaining input channels
    for (auto channel = 2; channel < totalNumInputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

//==============================================================================
void M1MonitorAudioProcessor::processStereoDownmix(juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);
    
    // Stereo Downmix Logic
    for (auto i = 0; i < totalNumInputChannels; ++i){
        scratchBuffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    }
    
    if (totalNumInputChannels == 4 || totalNumInputChannels == 8) {
        for (int channel = 0; channel < totalNumInputChannels; channel++) {
            for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
                outBufferL[sample] =+ scratchBuffer.getReadPointer(channel * 2    )[sample] / std::sqrt(2)/(totalNumInputChannels/2);
                outBufferR[sample] =+ scratchBuffer.getReadPointer(channel * 2 + 1)[sample] / std::sqrt(2)/(totalNumInputChannels/2);
            }
        }
    }
    else if (totalNumInputChannels == 12){
        //INDEX:          0,1,2,3,4,5, 6
        int mixMapL[] = { 0,2,4,6,8,10,11 };
        int mixMapR[] = { 1,3,5,7,8,9, 10 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (scratchBuffer.getReadPointer(mixMapL[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]/2
                + scratchBuffer.getReadPointer(mixMapL[6])[i])/std::sqrt(2)/(totalNumInputChannels/2);

            outBufferR[i] = (scratchBuffer.getReadPointer(mixMapR[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]
                + scratchBuffer.getReadPointer(mixMapL[6])[i]/2)/std::sqrt(2)/(totalNumInputChannels/2);
        }
    } else if (totalNumInputChannels == 14){
        //INDEX:          0,1,2,3,4,5, 6 ,7 ,8
        int mixMapL[] = { 0,2,4,6,8,10,11,12,13 };
        int mixMapR[] = { 1,3,5,7,8,9, 10,12,13 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (scratchBuffer.getReadPointer(mixMapL[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[6])[i]
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2)/std::sqrt(2)/(totalNumInputChannels/2);

            outBufferR[i] = (scratchBuffer.getReadPointer(mixMapR[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]
                + scratchBuffer.getReadPointer(mixMapL[6])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2)/std::sqrt(2)/(totalNumInputChannels/2);
        }
    } else if (totalNumInputChannels == 16){
        //INDEX:          0,1,2,3,4,5, 6 ,7 ,8 ,9
        int mixMapL[] = { 0,2,4,6,8,10,11,12,14,15 };
        int mixMapR[] = { 1,3,5,7,8,9, 10,12,13,14 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (
                  scratchBuffer.getReadPointer(mixMapL[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[6])[i]
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[9])[i]               )/std::sqrt(2)/(totalNumInputChannels/2);

            outBufferR[i] = (
                  scratchBuffer.getReadPointer(mixMapR[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]
                + scratchBuffer.getReadPointer(mixMapL[6])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]
                + scratchBuffer.getReadPointer(mixMapL[9])[i]/std::sqrt(2)/2)/std::sqrt(2)/(totalNumInputChannels/2);
        }
    } else if (totalNumInputChannels == 18){
        //INDEX:          0,1,2,3,4,5, 6 ,7 ,8 ,9 ,10,11
        int mixMapL[] = { 0,2,4,6,8,10,11,12,14,15,16,17 };
        int mixMapR[] = { 1,3,5,7,8,9, 10,12,13,14,16,17 };
        
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            outBufferL[i] = (
                  scratchBuffer.getReadPointer(mixMapL[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[6])[i]
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[9])[i]
                + scratchBuffer.getReadPointer(mixMapL[10])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[11])[i]/std::sqrt(2)/2)/std::sqrt(2)/(totalNumInputChannels/2);

            outBufferR[i] = (
                  scratchBuffer.getReadPointer(mixMapR[0])[i]
                + scratchBuffer.getReadPointer(mixMapL[1])[i]
                + scratchBuffer.getReadPointer(mixMapL[2])[i]
                + scratchBuffer.getReadPointer(mixMapL[3])[i]
                + scratchBuffer.getReadPointer(mixMapL[4])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[5])[i]
                + scratchBuffer.getReadPointer(mixMapL[6])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[7])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[8])[i]
                + scratchBuffer.getReadPointer(mixMapL[9])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[10])[i]/std::sqrt(2)/2
                + scratchBuffer.getReadPointer(mixMapL[11])[i]/std::sqrt(2)/2)/std::sqrt(2)/(totalNumInputChannels/2);
        }
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
