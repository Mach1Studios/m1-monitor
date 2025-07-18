#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::String M1MonitorAudioProcessor::paramYaw("yaw");
juce::String M1MonitorAudioProcessor::paramPitch("pitch");
juce::String M1MonitorAudioProcessor::paramRoll("roll");
juce::String M1MonitorAudioProcessor::paramMonitorMode("monitorMode");
juce::String M1MonitorAudioProcessor::paramOutputMode("outputMode");

//==============================================================================
M1MonitorAudioProcessor::M1MonitorAudioProcessor()
    : AudioProcessor(getHostSpecificLayout()),
      parameters(*this, &mUndoManager, juce::Identifier("M1-Monitor"), {
                                                                           std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramYaw, 1), TRANS("Yaw"), juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f), monitorSettings.yaw, "", juce::AudioProcessorParameter::genericParameter, [](float v, int) { return juce::String(v, 1) + "°"; }, [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                                                                           std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramPitch, 1), TRANS("Pitch"), juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.pitch, "", juce::AudioProcessorParameter::genericParameter, [](float v, int) { return juce::String(v, 1) + "°"; }, [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                                                                           std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(paramRoll, 1), TRANS("Roll"), juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), monitorSettings.roll, "", juce::AudioProcessorParameter::genericParameter, [](float v, int) { return juce::String(v, 1) + "°"; }, [](const juce::String& t) { return t.dropLastCharacters(3).getFloatValue(); }),
                                                                           std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramMonitorMode, 1), TRANS("Monitor Mode"), 0, 2, monitorSettings.monitor_mode),
                                                                           // Note: Change init output to max bus size when new formats are introduced
                                                                           std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramOutputMode, 1), TRANS("Output Mode"), 0, (int)Mach1DecodeMode::M1DecodeSpatial_14, (int)Mach1DecodeMode::M1DecodeSpatial_8),
                                                                       })
{
    parameters.addParameterListener(paramYaw, this);
    parameters.addParameterListener(paramPitch, this);
    parameters.addParameterListener(paramRoll, this);
    parameters.addParameterListener(paramMonitorMode, this);
    parameters.addParameterListener(paramOutputMode, this);

    // Setup for Mach1Decode API
    monitorSettings.m1Decode.setPlatformType(Mach1PlatformDefault);
    monitorSettings.m1Decode.setFilterSpeed(0.99);
    m1DecodeChangeInputMode(M1DecodeSpatial_8); // sets type and resizes temp buffers

    // We will assume the folders are properly created during the installation step
    juce::File settingsFile;
    // Using common support files installation location
    juce::File m1SupportDirectory = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);

    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0)
    {
        // test for any mac OS
        settingsFile = m1SupportDirectory.getChildFile("Application Support").getChildFile("Mach1");
    }
    else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0)
    {
        // test for any windows OS
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    }
    else
    {
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    }
    settingsFile = settingsFile.getChildFile("settings.json");
    DBG("Opening settings file: " + settingsFile.getFullPathName().quoted());

    // Informs OrientationManager that this client is expected to calculate the final orientation and to count instances for error handling
    m1OrientationClient.setClientType("monitor"); // Needs to be set before the init() function
    m1OrientationClient.initFromSettings(settingsFile.getFullPathName().toStdString());
    m1OrientationClient.setStatusCallback(std::bind(&M1MonitorAudioProcessor::setStatus, this, std::placeholders::_1, std::placeholders::_2));

    // setup OSC and the listener
    monitorOSC = std::make_unique<MonitorOSC>(this);
    monitorOSC->AddListener([&](juce::OSCMessage msg) {
        if (msg.getAddressPattern() == "/YPR-Offset")
        {
            float yaw = 0.0f;
            float pitch = 0.0f;
            //float roll = 0.0f;

            if (monitorSettings.yawActive && msg.size() >= 1)
                yaw = msg[0].getFloat32();
            if (monitorSettings.pitchActive && msg.size() >= 2)
                pitch = msg[1].getFloat32();
            //if (monitorSettings.rollActive && msg.size() >= 3) roll = msg[2].getFloat32();

            yaw = jmap(yaw, -180.0f, 180.0f, 0.0f, 1.0f);
            pitch = jmap(pitch, -90.0f, 90.0f, 0.0f, 1.0f);
            //roll = jmap(roll, -90.0f, 90.0f, 0.0f, 1.0f);

            yaw = std::fmodf(yaw, 1.0f);
            pitch = std::fmodf(pitch, 1.0f);
            //roll = std::fmodf(roll, 1.0f);

            if (yaw < 0)
                yaw += std::ceil(-yaw);
            if (pitch < 0)
                pitch += std::ceil(-pitch);
            //if (roll < 0) roll += std::ceil(-roll);

            parameters.getParameter(paramYaw)->setValueNotifyingHost(yaw);
            parameters.getParameter(paramPitch)->setValueNotifyingHost(pitch);
            //parameters.getParameter(paramRoll)->setValueNotifyingHost(roll);
        }
        else if (msg.getAddressPattern() == "/m1-channel-config")
        {
            DBG("[OSC] Recieved msg | Channel Config: " + std::to_string(msg[0].getInt32()));
            // Capturing monitor active state
            int channel_count = msg[0].getInt32();
            if (channel_count != monitorSettings.m1Decode.getFormatChannelCount()) // got a request for a different config
            {
                if (channel_count == 4)
                {
                    parameters.getParameter(paramOutputMode)->setValueNotifyingHost(parameters.getParameter(paramOutputMode)->convertTo0to1(Mach1DecodeMode::M1DecodeSpatial_4));
                }
                else if (channel_count == 8)
                {
                    parameters.getParameter(paramOutputMode)->setValueNotifyingHost(parameters.getParameter(paramOutputMode)->convertTo0to1(Mach1DecodeMode::M1DecodeSpatial_8));
                }
                else if (channel_count == 14)
                {
                    parameters.getParameter(paramOutputMode)->setValueNotifyingHost(parameters.getParameter(paramOutputMode)->convertTo0to1(Mach1DecodeMode::M1DecodeSpatial_14));
                }
                else
                {
                    DBG("[OSC] Error with received channel config!");
                }
            }
        }
        else
        {
            // display a captured unexpected osc message
            if (msg.size() > 0)
            {
                DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString());
                if (msg[0].isFloat32())
                {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + std::to_string(msg[0].getFloat32()));
                }
                else if (msg[0].isInt32())
                {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + std::to_string(msg[0].getInt32()));
                }
                else if (msg[0].isString())
                {
                    DBG("[OSC] Recieved unexpected msg | " + msg.getAddressPattern().toString() + ", " + msg[0].getString());
                }
            }
        }
    });

    // print build time for debug
    juce::String date(__DATE__);
    juce::String time(__TIME__);
    DBG("[MONITOR] Build date: " + date + " | Build time: " + time);

    // monitorOSC update timer loop (only used for checking the connection)
    startTimer(200);
}

M1MonitorAudioProcessor::~M1MonitorAudioProcessor()
{
    m1OrientationClient.command_disconnect();
    m1OrientationClient.close();
    jobThreads.addJob(new M1Analytics("M1-Monitor_Exited", (int)getSampleRate(), (int)monitorSettings.m1Decode.getFormatChannelCount(), m1OrientationClient.isConnectedToServer()), true);
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
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int M1MonitorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void M1MonitorAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String M1MonitorAudioProcessor::getProgramName(int index)
{
    return {};
}

void M1MonitorAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void M1MonitorAudioProcessor::createLayout()
{
    if (external_spatialmixer_active)
    {
        /// EXTERNAL MULTICHANNEL PROCESSING

        // INPUT
        if (monitorSettings.m1Decode.getFormatChannelCount() == 1)
        {
            getBus(true, 0)->setCurrentLayout(juce::AudioChannelSet::mono());
        }
        else if (monitorSettings.m1Decode.getFormatChannelCount() == 2)
        {
            getBus(true, 0)->setCurrentLayout(juce::AudioChannelSet::stereo());
        }
        // OUTPUT
        getBus(false, 0)->setCurrentLayout(juce::AudioChannelSet::stereo());
    }
    else
    {
        /// INTERNAL MULTICHANNEL PROCESSING

        // check if there is a mismatch of the current bus size on PT
        if (hostType.isProTools())
        {
            // update the monitorSettings if there is a mismatch

            // I/O Concept
            // Inputs: for this plugin we allow the m1Decode object to have a higher channel count input mode than what the host allows to support more configurations on channel specific hosts
            // Outputs: this is always stereo as the purpose of this plugin is the decode a stereo output stream via Mach1Decode API

            /// INPUTS
            if (getBus(true, 0)->getCurrentLayout().size() != monitorSettings.m1Decode.getFormatChannelCount())
            {
                if (getBus(true, 0)->getCurrentLayout().size() == 4)
                {
                    monitorSettings.m1Decode.setDecodeMode(Mach1DecodeMode::M1DecodeSpatial_4);
                    m1DecodeChangeInputMode(Mach1DecodeMode::M1DecodeSpatial_4);
                }
                else if (getBus(true, 0)->getCurrentLayout().size() == 8 || getBus(true, 0)->getCurrentLayout().getAmbisonicOrder() == 2)
                {
                    monitorSettings.m1Decode.setDecodeMode(Mach1DecodeMode::M1DecodeSpatial_8);
                    m1DecodeChangeInputMode(Mach1DecodeMode::M1DecodeSpatial_8);
                }
                else if (getBus(true, 0)->getCurrentLayout().size() == 14 || getBus(true, 0)->getCurrentLayout().getAmbisonicOrder() > 2)
                { // if an ambisonic bus higher than 2nd order
                    monitorSettings.m1Decode.setDecodeMode(Mach1DecodeMode::M1DecodeSpatial_14);
                    m1DecodeChangeInputMode(Mach1DecodeMode::M1DecodeSpatial_14);
                }
                else
                {
                    // an unsupported format
                }
            }
        }
    }
    layoutCreated = true; // flow control for static i/o
    updateHostDisplay();
}

//==============================================================================
void M1MonitorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Set up host bus layout
    if (!layoutCreated)
    {
        createLayout();
    }

    // flag for the initial reporting
    if (!pluginInitialized)
    {
        jobThreads.addJob(new M1Analytics("M1-Monitor_Launched", (int)getSampleRate(), (int)monitorSettings.m1Decode.getFormatChannelCount(), m1OrientationClient.isConnectedToServer()), true);
        pluginInitialized = true; // stop this from being called again
    }

    // can still be used to calculate coeffs even in STREAMING_PANNER_PLUGIN mode
    processorSampleRate = sampleRate;

    if (monitorSettings.m1Decode.getFormatChannelCount() != getMainBusNumInputChannels())
    {
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

void M1MonitorAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // incoming value expected as normalized
    // use this to update and sync parameter & monitorSettings
    if (parameterID == paramYaw)
    {
        monitorSettings.yaw = parameters.getParameter(paramYaw)->convertFrom0to1(parameters.getParameter(paramYaw)->getValue());
    }
    else if (parameterID == paramPitch)
    {
        monitorSettings.pitch = parameters.getParameter(paramPitch)->convertFrom0to1(parameters.getParameter(paramPitch)->getValue());
    }
    else if (parameterID == paramRoll)
    {
        monitorSettings.roll = parameters.getParameter(paramRoll)->convertFrom0to1(parameters.getParameter(paramRoll)->getValue());
    }
    else if (parameterID == paramMonitorMode)
    {
        monitorSettings.monitor_mode = (int)parameters.getParameter(paramMonitorMode)->convertFrom0to1(parameters.getParameter(paramMonitorMode)->getValue());

        // update gui on other plugins
        if (monitorOSC->isConnected() && monitorOSC->isActiveMonitor())
        {
            monitorOSC->sendMonitoringMode(monitorSettings.monitor_mode);
        }
    }
    else if (parameterID == paramOutputMode)
    {
        // `monitorSettings` are changed via the `m1DecodeChangeInputMode()` call
        m1DecodeChangeInputMode(Mach1DecodeMode((int)newValue));
        jobThreads.addJob(new M1Analytics("M1-Monitor_ModeChanged", (int)getSampleRate(), (int)monitorSettings.m1Decode.getFormatChannelCount(), m1OrientationClient.isConnectedToServer()), true);
    }

    // update the gui on other plugins for any changes to YPR
    if (parameterID == paramYaw || parameterID == paramPitch || parameterID == paramRoll)
    {
        if (monitorOSC->isConnected() && monitorOSC->isActiveMonitor())
        {
            // update the server and panners of final calculated orientation
            // sending un-normalized full range values in degrees
            monitorOSC->sendMasterYPR(monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll);
        }
    }

    // TODO: add UI for syncing panners to current monitor outputMode and add that outputMode int to this function
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool M1MonitorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet().isDisabled() ||
        layouts.getMainOutputChannelSet().isDisabled())
    {
        DBG("Layout REJECTED - Disabled buses");
        return false;
    }

    // If the host is Reaper always allow all configurations
    if (hostType.isReaper())
    {
        return true;
    }

    // For standalone, only allow stereo in/out
    if (JUCEApplicationBase::isStandaloneApp())
    {
        auto inputLayout = layouts.getMainInputChannelSet();
        auto outputLayout = layouts.getMainOutputChannelSet();

        bool isValid = (inputLayout == juce::AudioChannelSet::mono() ||
                        inputLayout == juce::AudioChannelSet::stereo() &&
                       outputLayout == juce::AudioChannelSet::stereo());

        DBG("Standalone Layout " + String(isValid ? "ACCEPTED" : "REJECTED") +
            " - Input: " + inputLayout.getDescription() +
            " Output: " + outputLayout.getDescription());

        return isValid;
    }

    // If the host is Pro Tools only allow the standard bus configurations
    if (hostType.isProTools())
    {
        bool validInput = (layouts.getMainInputChannelSet() == juce::AudioChannelSet::quadraphonic() ||
                           layouts.getMainInputChannelSet() == juce::AudioChannelSet::create7point1() ||
                           layouts.getMainInputChannelSet() == juce::AudioChannelSet::create7point1point6() ||
                           layouts.getMainInputChannelSet().getAmbisonicOrder() > 1);

        bool validOutput = (layouts.getMainOutputChannelSet().size() == juce::AudioChannelSet::stereo().size());

        DBG("Layout " + String(validInput && validOutput ? "ACCEPTED" : "REJECTED") +
            " - Input: " + layouts.getMainInputChannelSet().getDescription() +
            " Output: " + layouts.getMainOutputChannelSet().getDescription());

        return validInput && validOutput;
    }
    else if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo())
    {
        // RETURN TRUE FOR EXTERNAL STREAMING MODE
        // hard set {2,2} for streaming use case
        // TODO: Add a check for Reaper + EXTERNAL STREAMING MODE
        return true;
    }
    else
    {
        // Test for all available Mach1Encode configs
        // manually maintained for-loop of first enum element to last enum element
        Mach1Decode<float> configTester;
        for (int inputEnum = 0; inputEnum != Mach1DecodeMode::M1DecodeSpatial_14; inputEnum++)
        {
            configTester.setDecodeMode(static_cast<Mach1DecodeMode>(inputEnum));
            // test each input, if the input has the number of channels as the input testing layout has move on to output testing
            if (layouts.getMainInputChannelSet().size() >= configTester.getFormatChannelCount())
            {
                // test each output
                if (layouts.getMainOutputChannelSet().size() == juce::AudioChannelSet::stereo().size())
                {
                    return true;
                }
            }
        }
        return false;
    }
}
#endif

void M1MonitorAudioProcessor::fillChannelOrderArray(int numM1InputChannels)
{
    // sets the maximum channels of the current host layout
    juce::AudioChannelSet chanset = getBus(true, 0)->getCurrentLayout();
    int numHostInputChannels = getBus(true, 0)->getNumberOfChannels();

    // sets the maximum channels of the current selected m1 input layout
    std::vector<juce::AudioChannelSet::ChannelType> chan_types;
    chan_types.resize(numM1InputChannels);
    input_channel_indices.resize(numM1InputChannels);

    if (!chanset.isDiscreteLayout())
    { // Check for DAW specific instructions
        if (hostType.isProTools() && chanset.size() == 8 && chanset.getDescription().equalsIgnoreCase(juce::String("7.1 Surround")))
        {
            // TODO: Remove this and figure out why we cannot use what is in "else" on PT 7.1
            chan_types[0] = juce::AudioChannelSet::ChannelType::left;
            chan_types[1] = juce::AudioChannelSet::ChannelType::centre;
            chan_types[2] = juce::AudioChannelSet::ChannelType::right;
            chan_types[3] = juce::AudioChannelSet::ChannelType::leftSurroundSide;
            chan_types[4] = juce::AudioChannelSet::ChannelType::rightSurroundSide;
            chan_types[5] = juce::AudioChannelSet::ChannelType::leftSurroundRear;
            chan_types[6] = juce::AudioChannelSet::ChannelType::rightSurroundRear;
            chan_types[7] = juce::AudioChannelSet::ChannelType::LFE;
        }
        else
        {
            // Get the index of each channel supplied via JUCE
            for (int i = 0; i < numM1InputChannels; i++)
            {
                chan_types[i] = chanset.getTypeOfChannel(i);
            }
        }
        // Apply the index
        for (int i = 0; i < numM1InputChannels; i++)
        {
            input_channel_indices[i] = chanset.getChannelIndexForType(chan_types[i]);
        }
    }
    else
    { // is a discrete channel layout
        for (int i = 0; i < numM1InputChannels; ++i)
        {
            input_channel_indices[i] = i;
        }
    }
}

void M1MonitorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numInputChannels = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();

    // Safety check for buffer configuration
    if (buffer.getNumChannels() < 2)
    {
        DBG("Invalid buffer configuration: needs at least 2 channels");
        return;
    }

    // if you've got more output channels than input clears extra outputs
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    // m1_orientation_client update
    if (m1OrientationClient.isConnectedToServer())
    {
        syncParametersWithExternalOrientation();
    }

    // Mach1Decode processing loop
    monitorSettings.m1Decode.setRotationDegrees({ monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll });
    spatialMixerCoeffs = monitorSettings.m1Decode.decodeCoeffs();

    // Update spatial mixer coeffs from Mach1Decode for a smoothed value
    for (int channel = 0; channel < monitorSettings.m1Decode.getFormatChannelCount(); ++channel)
    {
        smoothedChannelCoeffs[channel][0].setTargetValue(spatialMixerCoeffs[channel * 2]); // Left output coeffs
        smoothedChannelCoeffs[channel][1].setTargetValue(spatialMixerCoeffs[channel * 2 + 1]); // Right output coeffs
    }

    juce::AudioBuffer<float> tempBuffer((std::max)(buffer.getNumChannels() * 2, monitorSettings.m1Decode.getFormatCoeffCount()), buffer.getNumSamples());
    tempBuffer.clear();

    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);

    if (getMainBusNumInputChannels() >= monitorSettings.m1Decode.getFormatChannelCount())
    {
        // TODO: Setup an else case for streaming input or error message

        // Internally downmix a stereo output stream and end the processBlock()
        if (monitorSettings.monitor_mode == 2)
        {
            processStereoDownmix(buffer);
            return;
        }

        // Setup buffers for Left & Right outputs
        for (auto channel = 0; channel < numInputChannels; ++channel)
        {
            tempBuffer.copyFrom(channel * 2, 0, buffer, channel, 0, buffer.getNumSamples());
            tempBuffer.copyFrom(channel * 2 + 1, 0, buffer, channel, 0, buffer.getNumSamples());
        }

        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {
            for (int input_channel = 0; input_channel < buffer.getNumChannels(); input_channel++)
            {
                buffer.getWritePointer(input_channel)[sample] = 0;
            }
        }

        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {
            for (int input_channel = 0; input_channel < monitorSettings.m1Decode.getFormatChannelCount(); input_channel++)
            {
                // We apply a channel re-ordering for DAW canonical specific Input channel configrations via fillChannelOrder() and `input_channel_reordered`
                // Input channel reordering from fillChannelOrder()
                int input_channel_reordered = input_channel_indices[input_channel];

                outBufferL[sample] += tempBuffer.getReadPointer(input_channel_reordered * 2)[sample] * smoothedChannelCoeffs[input_channel][0].getNextValue(); // all the even tempBuffer indices are for the left output
                outBufferR[sample] += tempBuffer.getReadPointer(input_channel_reordered * 2 + 1)[sample] * smoothedChannelCoeffs[input_channel][1].getNextValue(); // all the odd tempBuffer indices are for the right output
            }
        }
    }
    else
    {
        //        // Invalid Decode I/O; clear buffers
        //        // TODO: Prepare for input streamed case when needed here
        //        for (int channel = getTotalNumInputChannels(); channel <= getTotalNumOutputChannels(); ++channel)
        //            buffer.clear(channel, 0, buffer.getNumSamples());
    }

    // clear remaining input channels
    for (auto channel = 2; channel < getTotalNumInputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

//==============================================================================
void M1MonitorAudioProcessor::processStereoDownmix(juce::AudioBuffer<float>& buffer)
{
    auto numInputChannels = getMainBusNumInputChannels();
    auto numOutputChannels = getMainBusNumOutputChannels();

    juce::AudioBuffer<float> tempBuffer(numInputChannels * 2 + 2, buffer.getNumSamples());
    tempBuffer.clear();

    float* outBufferL = buffer.getWritePointer(0);
    float* outBufferR = buffer.getWritePointer(1);

    // Stereo Downmix Logic
    for (auto i = 0; i < numInputChannels; ++i)
    {
        tempBuffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    }

    if (numInputChannels == 4 || numInputChannels == 8)
    {
        for (int channel = 0; channel < numInputChannels; channel++)
        {
            for (int sample = 0; sample < buffer.getNumSamples(); sample++)
            {
                outBufferL[sample] += tempBuffer.getReadPointer(channel * 2)[sample] / std::sqrt(2) / (numInputChannels / 2);
                outBufferR[sample] += tempBuffer.getReadPointer(channel * 2 + 1)[sample] / std::sqrt(2) / (numInputChannels / 2);
            }
        }
    }
    else if (numInputChannels == 12)
    {
        //INDEX:          0,1,2,3,4,5, 6
        int mixMapL[] = { 0, 2, 4, 6, 8, 10, 11 };
        int mixMapR[] = { 1, 3, 5, 7, 8, 9, 10 };

        for (int i = 0; i < buffer.getNumSamples(); i++)
        {
            outBufferL[i] = (tempBuffer.getReadPointer(mixMapL[0])[i]
                                + tempBuffer.getReadPointer(mixMapL[1])[i]
                                + tempBuffer.getReadPointer(mixMapL[2])[i]
                                + tempBuffer.getReadPointer(mixMapL[3])[i]
                                + tempBuffer.getReadPointer(mixMapL[4])[i] / 2
                                + tempBuffer.getReadPointer(mixMapL[5])[i] / 2
                                + tempBuffer.getReadPointer(mixMapL[6])[i])
                            / std::sqrt(2) / (numInputChannels / 2);

            outBufferR[i] = (tempBuffer.getReadPointer(mixMapR[0])[i]
                                + tempBuffer.getReadPointer(mixMapL[1])[i]
                                + tempBuffer.getReadPointer(mixMapL[2])[i]
                                + tempBuffer.getReadPointer(mixMapL[3])[i]
                                + tempBuffer.getReadPointer(mixMapL[4])[i] / 2
                                + tempBuffer.getReadPointer(mixMapL[5])[i]
                                + tempBuffer.getReadPointer(mixMapL[6])[i] / 2)
                            / std::sqrt(2) / (numInputChannels / 2);
        }
    }
    else if (numInputChannels == 14)
    {
        //INDEX:          0,1,2,3,4,5, 6 ,7 ,8
        int mixMapL[] = { 0, 2, 4, 6, 8, 10, 11, 12, 13 };
        int mixMapR[] = { 1, 3, 5, 7, 8, 9, 10, 12, 13 };

        for (int i = 0; i < buffer.getNumSamples(); i++)
        {
            outBufferL[i] = (tempBuffer.getReadPointer(mixMapL[0])[i]
                                + tempBuffer.getReadPointer(mixMapL[1])[i]
                                + tempBuffer.getReadPointer(mixMapL[2])[i]
                                + tempBuffer.getReadPointer(mixMapL[3])[i]
                                + tempBuffer.getReadPointer(mixMapL[4])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[5])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[6])[i]
                                + tempBuffer.getReadPointer(mixMapL[7])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[8])[i] / std::sqrt(2) / 2)
                            / std::sqrt(2) / (numInputChannels / 2);

            outBufferR[i] = (tempBuffer.getReadPointer(mixMapR[0])[i]
                                + tempBuffer.getReadPointer(mixMapL[1])[i]
                                + tempBuffer.getReadPointer(mixMapL[2])[i]
                                + tempBuffer.getReadPointer(mixMapL[3])[i]
                                + tempBuffer.getReadPointer(mixMapL[4])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[5])[i]
                                + tempBuffer.getReadPointer(mixMapL[6])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[7])[i] / std::sqrt(2) / 2
                                + tempBuffer.getReadPointer(mixMapL[8])[i] / std::sqrt(2) / 2)
                            / std::sqrt(2) / (numInputChannels / 2);
        }
    }
}

//==============================================================================
void M1MonitorAudioProcessor::timerCallback()
{
    // Added if we need to move the OSC stuff from the processorblock
    monitorOSC->update(); // test for connection

    // transport
    updateTransportWithPlayhead(); // Updates here for hosts that freeze processBlock()
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
    auto* editor = new M1MonitorAudioProcessorEditor(*this);

    // When the processor sees a new alert, tell the editor to display it
    postAlertToUI = [editor](const Mach1::AlertData& a)
    {
        editor->monitorUIBaseComponent->postAlert(a);
    };

    return editor;
}

//==============================================================================
void M1MonitorAudioProcessor::m1DecodeChangeInputMode(Mach1DecodeMode decodeMode)
{
    if (monitorSettings.m1Decode.getDecodeMode() != decodeMode)
    {
        DBG("Current config: " + std::to_string(monitorSettings.m1Decode.getDecodeMode()) + " and new config: " + std::to_string(decodeMode));
        monitorSettings.m1Decode.setDecodeMode(decodeMode);
        // Report change to m1-system-helper for other clients and plugins
        monitorOSC->sendRequestToChangeChannelConfig(monitorSettings.m1Decode.getFormatChannelCount());
    }

    auto inputChannelsCount = monitorSettings.m1Decode.getFormatChannelCount();
    smoothedChannelCoeffs.resize(inputChannelsCount);

    // Checks if input bus is non DISCRETE layout and fixes host specific channel ordering issues
    fillChannelOrderArray(monitorSettings.m1Decode.getFormatChannelCount());

    for (int input_channel = 0; input_channel < inputChannelsCount; input_channel++)
    {
        smoothedChannelCoeffs[input_channel].resize(2);
        for (int output_channel = 0; output_channel < 2; output_channel++)
        {
            smoothedChannelCoeffs[input_channel][output_channel].reset(processorSampleRate, (double)0.01);
        }
    }
}

//==============================================================================
void M1MonitorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // This method is used to store your parameters in the memory block.

    // Create an XML doc from the parameters
    std::unique_ptr<juce::XmlElement> parameters_xml(parameters.state.createXml());

    // Append the Timecode Offset settings
    parameters_xml->setAttribute("HH", monitorOSC->HH);
    parameters_xml->setAttribute("MM", monitorOSC->MM);
    parameters_xml->setAttribute("SS", monitorOSC->SS);
    parameters_xml->setAttribute("FS", monitorOSC->FS);

    // Append extra plugin settings
    parameters_xml->setAttribute("isActive", monitorOSC->isActiveMonitor());

    // Save to output memory
    copyXmlToBinary(*parameters_xml, destData);
}

void M1MonitorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // This method is used to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        if (xml->hasTagName(parameters.state.getType()))
        {
            juce::ValueTree tree = juce::ValueTree::fromXml(*xml);
            if (tree.isValid())
            {
                parameters.state = tree;

                // Force update the input/output temp buffers
                m1DecodeChangeInputMode(Mach1DecodeMode((int)parameters.getParameter(paramOutputMode)->convertFrom0to1(parameters.getParameter(paramOutputMode)->getValue())));
            }

            // Set the timecode offset from the saved plugin data
            monitorOSC->HH = xml->getIntAttribute("HH");
            monitorOSC->MM = xml->getIntAttribute("MM");
            monitorOSC->SS = xml->getIntAttribute("SS");
            monitorOSC->FS = xml->getIntAttribute("FS");

            // Append extra plugin settings
            monitorOSC->setActiveState(xml->getIntAttribute("isActive"));
        }
    }
}

void M1MonitorAudioProcessor::updateTransportWithPlayhead()
{
    AudioPlayHead* ph = getPlayHead();

    if (ph == nullptr)
    {
        return;
    }

    auto play_head_position = ph->getPosition();

    if (!play_head_position.hasValue())
    {
        return;
    }

    if (play_head_position->getTimeInSeconds().hasValue())
    {
        monitorOSC->setTimeInSeconds(*play_head_position->getTimeInSeconds());
    }

    monitorOSC->setIsPlaying(ph->getPosition()->getIsPlaying());
}

void M1MonitorAudioProcessor::syncParametersWithExternalOrientation()
{
    monitorOSC->sendMonitoringMode(monitorSettings.monitor_mode);
    monitorOSC->sendMasterYPR(monitorSettings.yaw, monitorSettings.pitch, monitorSettings.roll);

    // Get the change in the orientation, provided by the external device, in degrees. (ex_ori_delta_deg)
    auto ex_ori_vec = m1OrientationClient.getOrientation().GetGlobalRotationAsEulerDegrees();
    auto ext_ori_delta_vec = ex_ori_vec - m_last_external_ori_degrees;
    m_last_external_ori_degrees = ex_ori_vec;

    // Zero out changes in locked axes.
    auto axis_locks_vec = Mach1::Float3{
        monitorSettings.yawActive ? 1.0f : 0.0f,
        monitorSettings.pitchActive ? 1.0f : 0.0f,
        monitorSettings.rollActive ? 1.0f : 0.0f
    };
    ext_ori_delta_vec *= axis_locks_vec;

    if (ext_ori_delta_vec.IsApproximatelyEqual({ 0.0f }))
        return; // nothing happened

    // Convert the vector of change in degrees into a vector of change in VST parameter values.
    // That way, we don't need to fiddle around with differing 3D vector component convention or the fact that
    // each component could have a different range [(0->360; -90->90), but they are provided by the client as (-180 to 180)].
    // Keep in mind that change in pitch and roll are halved, as their range is half as small.
    ext_ori_delta_vec = ext_ori_delta_vec.Map(0, 360, 0, 1);

    // Get the current state of the orientation, as normalized values. Zero out locked axes.
    auto yaw = parameters.getParameter(paramYaw)->getValue() * axis_locks_vec.GetYaw();
    auto pitch = parameters.getParameter(paramPitch)->getValue() * axis_locks_vec.GetPitch();
    auto roll = parameters.getParameter(paramRoll)->getValue() * axis_locks_vec.GetRoll();

    // Manually apply the change in parameters, modulating when size is > 1. Compensate for pitch and roll.
    yaw = std::fmodf(yaw + ext_ori_delta_vec.GetYaw(), 1.0f);
    pitch = std::fmodf(pitch + ext_ori_delta_vec.GetPitch() * 2.0f, 1.0f);
    roll = std::fmodf(roll + ext_ori_delta_vec.GetRoll() * 2.0f, 1.0f);

    // Modulate when size is < 0.
    if (yaw < 0)
        yaw += std::ceil(-yaw);
    if (pitch < 0)
        pitch += std::ceil(-pitch);
    if (roll < 0)
        roll += std::ceil(-roll);

    // Notify the parameters of their new state.
    parameters.getParameter(paramYaw)->setValueNotifyingHost(yaw);
    parameters.getParameter(paramPitch)->setValueNotifyingHost(pitch);
    parameters.getParameter(paramRoll)->setValueNotifyingHost(roll);
}

//==============================================================================
void M1MonitorAudioProcessor::addJob(std::function<void()> job)
{
    jobThreads.addJob(std::move(job));
}

void M1MonitorAudioProcessor::addJob(juce::ThreadPoolJob* job, bool deleteJobWhenFinished)
{
    jobThreads.addJob(job, deleteJobWhenFinished);
}

void M1MonitorAudioProcessor::cancelJob(juce::ThreadPoolJob* job)
{
    jobThreads.removeJob(job, true, 100);
}

juce::ThreadPool& M1MonitorAudioProcessor::getThreadPool()
{
    return jobThreads;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new M1MonitorAudioProcessor();
}

void M1MonitorAudioProcessor::postAlert(const Mach1::AlertData& alert)
{
    if (postAlertToUI) {
        postAlertToUI(alert);
    } else {
        pendingAlerts.push_back(alert); // Store for later
        DBG("Stored alert for UI. Total pending: " + juce::String(pendingAlerts.size()));
    }
}
