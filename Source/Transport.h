#pragma once

#include <JuceHeader.h>

#include "m1_orientation_client/M1OrientationClient.h"

class Transport : public juce::Timer
{
public:

    Transport(M1OrientationClient* orientationClient);

    void setOffset(int hh, int mm, int ss, int fs);

    double getTimeInSeconds() const;

    void setTimeInSeconds(double time_in_seconds);

    bool getIsPlaying() const;

    void setIsPlaying( bool is_playing);

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
