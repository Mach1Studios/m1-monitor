#pragma once

#include <JuceHeader.h>

class MonitorOSC : private juce::OSCSender, private juce::OSCReceiver, private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
    bool init(int helperPort);
    bool initFromSettings(std::string jsonSettingsFilePath);
	int helperPort = 0, port = 0;
	bool isConnected = false; // used to track connection with helper utility
    bool isActiveMonitor = true; // used to track if this is the primary monitor instance
	std::function<void(juce::OSCMessage msg)> messageReceived;
	void oscMessageReceived(const juce::OSCMessage& msg) override;
    juce::uint32 lastMessageTime = 0; 

public:
    MonitorOSC();
	~MonitorOSC();

	void update();
	void AddListener(std::function<void(juce::OSCMessage msg)> messageReceived);
	bool Send(const juce::OSCMessage& msg);
	bool IsConnected();
    
    bool sendRequestToBecomeActive();
    void setActiveState(bool is_active);
    bool IsActiveMonitor();
    
    bool sendMonitoringMode(int input_mode);
    bool sendMasterYPR(float yaw, float pitch, float roll);
    
    bool connectToHelper();
    bool disconnectToHelper();
};
