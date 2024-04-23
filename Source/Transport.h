#pragma once

#include <JuceHeader.h>

#include "m1_orientation_client/M1OrientationClient.h"

class Transport : public juce::Timer
{
public:

    Transport(M1OrientationClient* orientationClient);

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
     * @brief Send over the current state of this Transport via its M1OrientationClient.
     */
    void sendData();

    void timerCallback() override;

public:
    int HH = 0;
    int MM = 0;
    int SS = 0;
    int FS = 0;

private:
	M1OrientationClient* m_orientation_client = nullptr;

    double m_transport_offset = 0;
	double m_correct_time_in_seconds = 0;
    bool m_is_playing = true;
};
