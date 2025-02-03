#pragma once

#include <JuceHeader.h>
#include "AlertData.h"

class M1MonitorAudioProcessor;

class MonitorOSC : public juce::DeletedAtShutdown, public juce::Timer, private juce::OSCSender, private juce::OSCReceiver, private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    M1MonitorAudioProcessor* processor = nullptr;
    MonitorOSC(M1MonitorAudioProcessor* processor);
    ~MonitorOSC();

    // OSC
    bool init(int helperPort);
    bool initFromSettings(const std::string& jsonSettingsFilePath);
    int helperPort = 0, port = 0;
    std::function<void(juce::OSCMessage msg)> messageReceived;
    void oscMessageReceived(const juce::OSCMessage& msg) override;
    juce::uint32 lastMessageTime = 0;

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

    // Transport
    /**
     * @brief Get the current time in the transport in seconds
     */
    double getTimeInSeconds() const;

    /**
     * @brief Set the current time in the transport in seconds
     */
    void setTimeInSeconds(double time_in_seconds);

    /**
     * @brief Get whether playback is current active
     */
    bool getIsPlaying() const;

    /**
     * @brief Set whether playback is current active
     */
    void setIsPlaying(bool is_playing);

    /**
     * @brief Send a specific framerate to player
     */
    bool command_setPlayerFrameRate(float playerFrameRate);

    /**
     * @brief Send the position in seconds of playhead in DAW to player
     */
    bool command_setPlayerPositionInSeconds(float playerPlayheadPositionInSeconds);

    /**
     * @brief Send the current playstate of the DAW to the player
     */
    bool command_setPlayerIsPlaying(bool playerIsPlaying);

    /**
     * @brief Send over the current state of this Transport via its M1OrientationClient.
     */
    void sendData();

    void timerCallback() override;

    int HH = 0;
    int MM = 0;
    int SS = 0;
    int FS = 0;

private:
    // OSC
    bool is_connected = false; // used to track connection with helper utility
    bool is_active_monitor = true; // used to track if this is the primary monitor instance

    // Transport
    double m_transport_offset = 0;
    double m_correct_time_in_seconds = 0;
    double m_last_sent_position = -1.0f; // Initialize to invalid value to ensure first send
    bool m_is_playing = false;
    bool m_last_sent_playing_state = false;
    int lastUpdateToPlayer = 0;
};
