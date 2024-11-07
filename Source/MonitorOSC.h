#pragma once

#include <JuceHeader.h>

class MonitorOSC : private juce::OSCSender, private juce::OSCReceiver, private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    bool init(int helperPort);
    bool initFromSettings(std::string jsonSettingsFilePath);
    int helperPort = 0, port = 0;
    std::function<void(juce::OSCMessage msg)> messageReceived;
    void oscMessageReceived(const juce::OSCMessage& msg) override;
    juce::uint32 lastMessageTime = 0;

private:
    bool is_connected = false; // used to track connection with helper utility
    bool is_active_monitor = true; // used to track if this is the primary monitor instance

public:
    MonitorOSC();
    ~MonitorOSC();

    void update();
    void AddListener(std::function<void(juce::OSCMessage msg)> messageReceived);
    bool Send(const juce::OSCMessage& msg);
    bool isConnected();

    bool sendRequestToBecomeActive();
    bool sendRequestToChangeChannelConfig(int channel_count_for_config);
    void setActiveState(bool is_active);
    bool isActiveMonitor();

    bool sendMonitoringMode(int input_mode);
    bool sendMasterYPR(float yaw, float pitch, float roll);

    bool connectToHelper();
    bool disconnectToHelper();
};
