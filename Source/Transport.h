/*
  ==============================================================================

    Transport.h
    Created: 11 Oct 2022 3:25:40pm
    Author:  Dylan Marcus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Transport : public juce::Timer
{
public:
    Transport();

    // Offset timecode
    int HH = 0, MM = 0, SS = 0, FS = 0;

    void update();
    void setProcessor(AudioProcessor* processor);
    void setOrientationClient(M1OrientationOSCClient* m1OrientationOSCClient);
    void updateOffset(int hh, int mm, int ss, int fs);
    double getTimeInSeconds() {
        return currentTimeInSeconds;
    }

private:
    void updateCurrentTimeInfoFromHost();
    void sendDataViaOrientationClient();
    void showConnectionErrorMessage(String msg);

    double lastSentPositionValue;
    bool lastSentIsPlaying;

    void timerCallback() override;

    juce::AudioPlayHead::CurrentPositionInfo lastPosInfo;
    double currentTimeInSeconds;
    float currentFrameRate;;

    juce::AudioProcessor* processor = nullptr;
    M1OrientationOSCClient* m1OrientationClient = nullptr;
};
