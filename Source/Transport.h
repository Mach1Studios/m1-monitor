#pragma once

#include <JuceHeader.h>

class Transport : public juce::Timer
{
public:
	Transport(M1OrientationClient* orientationClient);

	// Offset timecode
	int HH = 0, MM = 0, SS = 0, FS = 0;

	void update();
	void setProcessor(AudioProcessor* processor);

	void updateOffset(int hh, int mm, int ss, int fs);
    
    double getTimeInSeconds() {
        return correctTimeInSeconds;
    }

private:
	M1OrientationClient* orientationClient = nullptr;

	void updateCurrentTimeInfoFromHost();
	void sendData();
	void showConnectionErrorMessage(String msg);

	double lastSentPositionValue;

	void timerCallback() override;

	juce::AudioPlayHead::CurrentPositionInfo lastPosInfo;
	double correctTimeInSeconds;

    juce::String reqResult;

 	int packetsSent = 0;
	int transportOffset = 0;

    juce::AudioProcessor* processor = nullptr;
};
