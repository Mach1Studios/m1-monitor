#ifndef TRANSPORT_H_INCLUDED
#define TRANSPORT_H_INCLUDED

#include <JuceHeader.h>

class Transport : public juce::Timer
{
public:
	Transport();

	// Offset timecode
	int HH = 0, MM = 0, SS = 0, FS = 0;

	void update();
	void setProcessor(AudioProcessor* processor);

	void updateOffset(int hh, int mm, int ss, int fs);
    
    double getTimeInSeconds() {
        return correctTimeInSeconds;
    }

private:

	void updateCurrentTimeInfoFromHost();
	void sendDataViaOsc();
	void showConnectionErrorMessage(String msg);

	double lastSentPositionValue;

	void timerCallback() override;

	juce::AudioPlayHead::CurrentPositionInfo lastPosInfo;
	double correctTimeInSeconds;

    juce::String reqResult;

    juce::OSCSender sender;
	int packetsSent = 0;

    juce::AudioProcessor* processor = nullptr;
};

#endif  // TRANSPORT_H_INCLUDED
