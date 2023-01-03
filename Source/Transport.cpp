/*
  ==============================================================================

    Transport.cpp
    Created: 11 Oct 2022 3:25:40pm
    Author:  Dylan Marcus

  ==============================================================================
*/

#include "Transport.h"

Transport::Transport()
{
//    if (!sender.connect("127.0.0.1", 9001)) {
//        showConnectionErrorMessage("Error: could not connect to UDP port 9001.");
//    }

    HH = 0;
    MM = 0;
    SS = 0;
    FS = 0;

    startTimer(50);
}

void Transport::updateOffset(int hh, int mm, int ss, int fs) {
    if ((hh != HH) || (mm != MM) || (ss != SS) || (fs != FS)) {
        HH = hh;
        MM = mm;
        SS = ss;
        FS = fs;
        if (m1OrientationClient->isConnectedToServer()) {
            m1OrientationClient->command_setFrameRate(currentFrameRate);
            m1OrientationClient->command_setPlayheadPositionInSeconds((hh*60+mm)*60+ss+(fs/currentFrameRate));
        }
    }
}

void Transport::sendDataViaOrientationClient() {
    if (processor != nullptr) {
        if (m1OrientationClient != nullptr) {
            if (m1OrientationClient->isConnectedToServer()) {
                m1OrientationClient->command_setFrameRate(currentFrameRate);
                m1OrientationClient->command_setPlayheadPositionInSeconds((float)currentTimeInSeconds);
                //m.addInt32((lastPosInfo.isPlaying ? 1 : 0));
                lastSentPositionValue = lastPosInfo.timeInSeconds;
            }
        }
    }
}

void Transport::timerCallback() {
    updateCurrentTimeInfoFromHost();
    sendDataViaOrientationClient();
}

void Transport::update() {
    updateCurrentTimeInfoFromHost();
}

void Transport::setProcessor(AudioProcessor* processor) {
    this->processor = processor;
}

void Transport::setOrientationClient(M1OrientationOSCClient* m1OrientationOSCClient) {
    this->m1OrientationClient = m1OrientationOSCClient;
}

void Transport::updateCurrentTimeInfoFromHost() {
    if (processor != nullptr) {
        if (AudioPlayHead* ph = processor->getPlayHead()) {
            AudioPlayHead::CurrentPositionInfo newTime;
            processor->getSampleRate();
            
            if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps23976) {
                currentFrameRate = 23.976;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps24) {
                currentFrameRate = 24;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps25) {
                currentFrameRate = 25;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps2997) {
                currentFrameRate = 29.97;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps30) {
                currentFrameRate = 30;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps2997drop) {
                currentFrameRate = 29.97;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps30drop) {
                currentFrameRate = 30;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps60) {
                currentFrameRate = 60;
            } else if (newTime.frameRate == juce::AudioPlayHead::FrameRateType::fps60drop) {
                currentFrameRate = 60;
            } else {
                // framerate unknown
            }

            if (ph->getCurrentPosition(newTime)) {
                lastPosInfo = newTime;  // Successfully got the current time from the host..
                //correctTimeInSeconds = (lastPosInfo.ppqPosition / newTime.bpm) * 60;
                currentTimeInSeconds = lastPosInfo.timeInSeconds;
                return;
            }
        }

        // If the host fails to provide the current time, we'll just reset our copy to a default..
        lastPosInfo.resetToDefault();
    }
}

void Transport::showConnectionErrorMessage(String msg) {
    AlertWindow::showMessageBoxAsync(
        AlertWindow::WarningIcon,
        "Connection error",
        msg,
        "OK");
}
