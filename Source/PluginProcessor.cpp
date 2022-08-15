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
    m1decode.setPlatformType(Mach1PlatformDefault);
    m1decode.setDecodeAlgoType(Mach1DecodeAlgoSpatial_8);
    m1decode.setFilterSpeed(0.99);
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
    smoothedChannelCoeffs.resize(m1decode.getFormatCoeffCount());
    spatialMixerCoeffs.resize(m1decode.getFormatCoeffCount());
    for (int input_channel = 0; input_channel < m1decode.getFormatChannelCount(); input_channel++) {
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

    // if you've got more output channels than input clears extra outputs
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Mach1Decode processing loop
    Mach1Point3D currentOrientation;
    // retrieve normalized values
    (parameters.getParameter(paramYawEnable)->getValue()) ? currentOrientation.x = parameters.getParameter(paramYaw)->getValue() : currentOrientation.x = 0.0f;
    (parameters.getParameter(paramPitchEnable)->getValue()) ? currentOrientation.y = parameters.getParameter(paramPitch)->getValue() : currentOrientation.y = 0.0f;
    (parameters.getParameter(paramRollEnable)->getValue()) ? currentOrientation.z = parameters.getParameter(paramRoll)->getValue() : currentOrientation.z = 0.0f;
    m1decode.setRotation(currentOrientation);
    m1decode.beginBuffer();
    spatialMixerCoeffs = m1decode.decodeCoeffs();
    m1decode.endBuffer();
    
    for (int input_channel = 0; input_channel < totalNumInputChannels; ++input_channel) {
        smoothedChannelCoeffs[input_channel * 2].setTargetValue(spatialMixerCoeffs[input_channel * 2]);
        smoothedChannelCoeffs[input_channel * 2 + 1].setTargetValue(spatialMixerCoeffs[input_channel * 2 + 1]);
    }
    
    if (totalNumInputChannels == m1decode.getFormatChannelCount()){ // dumb safety check, TODO: do better i/o error handling
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {
            for (int input_channel = 0; input_channel < totalNumInputChannels; ++input_channel) {
                auto inputChannelBuffer = buffer.getReadPointer(input_channel, sample);
                auto outSampleL = buffer.getWritePointer(input_channel, sample);
                auto outSampleR = buffer.getWritePointer(input_channel, sample);
                
                *outSampleL = *inputChannelBuffer * smoothedChannelCoeffs[input_channel * 2].getNextValue();
                *outSampleR = *inputChannelBuffer * smoothedChannelCoeffs[input_channel * 2 + 1].getNextValue();
            }
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
