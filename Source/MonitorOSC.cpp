#include "MonitorOSC.h"

#include "PluginProcessor.h"

MonitorOSC::MonitorOSC(M1MonitorAudioProcessor* processor_)
{
    processor = processor_;
    is_connected = false;

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

    initFromSettings(settingsFile.getFullPathName().toStdString());
    juce::OSCReceiver::addListener(this);

    startTimer(10); // 10ms timer for playhead position updates
}

MonitorOSC::~MonitorOSC()
{
    if (is_connected && helperPort > 0)
    {
        disconnectToHelper();
    }
    juce::OSCSender::disconnect();
    juce::OSCReceiver::disconnect();

    // reset the port
    port = 0;
}

bool MonitorOSC::init(int helperPort)
{
    // check port
    juce::DatagramSocket socket(false);
    socket.setEnablePortReuse(false);
    this->helperPort = helperPort;

    // find available port
    for (port = 10201; port < 10300; port++)
    {
        if (socket.bindToPort(port))
        {
            socket.shutdown(); // shutdown port to not block the juce::OSCReceiver::connect return
            juce::OSCReceiver::connect(port);
            break; // stops the incrementing on the first available port
        }
    }

    if (port > 10200)
    {
        return true;
    }
}

// finds the server port via the settings json file
bool MonitorOSC::initFromSettings(const std::string& jsonSettingsFilePath)
{
    juce::File settingsFile(jsonSettingsFilePath);

    if (!settingsFile.exists())
    {
        if (processor)
        {
            Mach1::AlertData data { "Warning", "The settings.json file doesn't exist in Mach1's Application Support directory!\nPlease reinstall the Mach1 Spatial System.", "OK" };
            processor->postAlert(data);
        }
        return false;
    }
    else
    {
        juce::var mainVar = juce::JSON::parse(juce::File(jsonSettingsFilePath));
        int helperPort = mainVar["helperPort"];

        if (!init(helperPort))
        {
            if (processor)
            {
                Mach1::AlertData data { "Warning", "There is a port conflict, please choose a new port", "OK" };
                processor->postAlert(data);
            }
        }
    }
    return true;
}

void MonitorOSC::oscMessageReceived(const juce::OSCMessage& msg)
{
    if (messageReceived != nullptr)
    {
        if (msg.getAddressPattern() == "/connectedToServer")
        {
            is_connected = true;
        }
        else if (msg.getAddressPattern() == "/m1-activate-client")
        {
            DBG("[OSC] Received msg | Activate: " + std::to_string(msg[0].getInt32()));
            // Capturing monitor active state
            int active = msg[0].getInt32();
            if (active == 1)
            {
                setActiveState(true);
            }
            else if (active == 0)
            {
                setActiveState(false);
            }
            is_connected = true;
        }
        else if (msg.getAddressPattern() == "/m1-response")
        {
            is_connected = true; // ping response from m1-status
        }
        else if (msg.getAddressPattern() == "/m1-reconnect-req")
        {
            disconnectToHelper();
            is_connected = false;
        }
        else if (msg.getAddressPattern() == "/m1-helper-port-changed") {
            if (msg.size() >= 1 && msg[0].isInt32()) {
                int newHelperPort = msg[0].getInt32();
                DBG("[OSC] Helper port changed to: " + std::to_string(newHelperPort));

                // Update our stored helper port
                helperPort = newHelperPort;

                // Disconnect and reconnect with the new port
                disconnectToHelper();
                is_connected = false;

                // Don't reconnect immediately - let the update() method handle it
            }
        }
        else
        {
            messageReceived(msg);
        }
    }
    lastMessageTime = juce::Time::getMillisecondCounter();
}

void MonitorOSC::update()
{
    if (!is_connected && helperPort > 0)
    {
        connectToHelper();
    }

    if (is_connected)
    {
        // updates pingtime on helper tool
        juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/m1-status"));
        m.addInt32(port); // port used for id
        juce::OSCSender::send(m); // check to update isConnected for error catching;

        // signals disconnect if helper is not found
        juce::uint32 currentTime = juce::Time::getMillisecondCounter();
        if ((currentTime - lastMessageTime) > 10000)
        { // 10000 milliseconds = 10 seconds
            if (helperPort > 0)
            {
                disconnectToHelper();
            }
            is_connected = false;
        }
    }
}

void MonitorOSC::AddListener(std::function<void(juce::OSCMessage msg)> messageReceived)
{
    this->messageReceived = messageReceived;
}

bool MonitorOSC::Send(const juce::OSCMessage& msg)
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            is_connected = juce::OSCSender::send(msg);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

bool MonitorOSC::isConnected()
{
    return is_connected;
}

bool MonitorOSC::isActiveMonitor()
{
    return is_active_monitor;
}

void MonitorOSC::setActiveState(bool is_active)
{
    is_active_monitor = is_active;
}

bool MonitorOSC::sendRequestToBecomeActive()
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setMonitorActiveReq"));
            m.addInt32(port); // int of current monitor port to use for identification
            is_connected = juce::OSCSender::send(m);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

bool MonitorOSC::sendRequestToChangeChannelConfig(int channel_count_for_config)
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setChannelConfigReq"));
            m.addInt32(channel_count_for_config); // int of new layout
            is_connected = juce::OSCSender::send(m);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

bool MonitorOSC::sendMonitoringMode(int mode)
{
    if (is_connected && port > 0)
    {
        juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setMonitoringMode"));
        m.addInt32(mode); // int of monitor mode
        return juce::OSCSender::send(m); // check to update isConnected for error catching;
    }
    return false;
}

bool MonitorOSC::sendMasterYPR(float yaw, float pitch, float roll)
{
    if (is_connected && port > 0)
    {
        juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setMasterYPR"));
        m.addFloat32(yaw); // expected degrees -180->180
        m.addFloat32(pitch); // expected degrees -90->90
        m.addFloat32(roll); // expected degrees -90->90
        return juce::OSCSender::send(m); // check to update isConnected for error catching
    }
    return false;
}

bool MonitorOSC::connectToHelper()
{
    // this first if statement protects against the debugger catching the wrong instance
    if ((this->port > 100 && this->port < 65535) && (this->helperPort > 100 && this->helperPort < 65535))
    {
        try
        {
            if (juce::OSCSender::connect("127.0.0.1", helperPort))
            {
                juce::OSCMessage msg = juce::OSCMessage(juce::OSCAddressPattern("/m1-addClient"));
                msg.addInt32(port);
                msg.addString("monitor");
                DBG("[OSC] Monitor registered as: " + std::to_string(port));
                is_connected = juce::OSCSender::send(msg);
                return is_connected;
            }
        }
        catch (...)
        {
            DBG("[OSC] Failed to connect to helper");
            is_connected = false;
        }
    }
    return false;
}

bool MonitorOSC::disconnectToHelper()
{
    // this first if statement protects against the debugger catching the wrong instance
    if ((this->port > 100 && this->port < 65535) && (this->helperPort > 100 && this->helperPort < 65535))
    {
        try
        {
            if (juce::OSCSender::connect("127.0.0.1", helperPort))
            {
                juce::OSCMessage msg = juce::OSCMessage(juce::OSCAddressPattern("/m1-removeClient"));
                msg.addInt32(port);
                msg.addString("monitor");
                DBG("[OSC] Unregistered: " + std::to_string(port));
                is_connected = juce::OSCSender::send(msg);
                return is_connected;
            }
        }
        catch (...)
        {
            DBG("[OSC] Failed to disconnect from helper");
            is_connected = false;
        }
    }
    return false;
}

double MonitorOSC::getTimeInSeconds() const
{
    return m_correct_time_in_seconds;
}

void MonitorOSC::setTimeInSeconds(double time_in_seconds)
{
    m_correct_time_in_seconds = time_in_seconds;
}

bool MonitorOSC::getIsPlaying() const
{
    return m_is_playing;
}

void MonitorOSC::setIsPlaying(bool is_playing)
{
    m_is_playing = is_playing;
}

bool MonitorOSC::command_setPlayerFrameRate(float playerFrameRate)
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setPlayerFrameRate"));
            m.addFloat32(playerFrameRate);
            is_connected = juce::OSCSender::send(m);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

bool MonitorOSC::command_setPlayerPositionInSeconds(float playerPlayheadPositionInSeconds)
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            lastUpdateToPlayer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setPlayerPosition"));
            m.addInt32(lastUpdateToPlayer);
            m.addFloat32(playerPlayheadPositionInSeconds);
            m_last_sent_position = playerPlayheadPositionInSeconds;
            is_connected = juce::OSCSender::send(m);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

bool MonitorOSC::command_setPlayerIsPlaying(bool playerIsPlaying)
{
    if (is_connected && port > 0)
    {
        // Try to reconnect if needed
        if (!juce::OSCSender::connect("127.0.0.1", helperPort))
        {
            is_connected = false;
            return false;
        }

        try
        {
            lastUpdateToPlayer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setPlayerIsPlaying"));
            m.addInt32(lastUpdateToPlayer);
            m.addInt32(playerIsPlaying);
            m_last_sent_playing_state = playerIsPlaying;
            is_connected = juce::OSCSender::send(m);
        }
        catch (...)
        {
            is_connected = false;
            return false;
        }
        return is_connected;
    }
    return false;
}

void MonitorOSC::sendData()
{
    if (!isConnected() || !isActiveMonitor())
        return;

    m_transport_offset = HH * 1000 * 60 * 60 + MM * 1000 * 60 + SS * 1000 + FS;
    auto play_head_position = m_correct_time_in_seconds - m_transport_offset / 1000;
    if (play_head_position < 0)
        play_head_position = 0;

    if (std::abs(play_head_position - m_last_sent_position) > 0.001f)
    {
        if (!command_setPlayerPositionInSeconds(play_head_position))
        {
            DBG("[OSC] Error sending playhead position to helper.");
        }
    }

    if (getIsPlaying() != m_last_sent_playing_state)
    {
        if (!command_setPlayerIsPlaying(getIsPlaying()))
        {
            DBG("[OSC] Error sending playstate to helper.");
        }
    }
}

void MonitorOSC::timerCallback()
{
    // only send the data if this is the active monitor instance to reduce conflict
    if (isConnected() && isActiveMonitor())
    {
        sendData();
    }
}
