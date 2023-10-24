
#include "Transport.h"

//==============================================================================
Transport::Transport(M1OrientationClient* orientationClient)
{
	HH = 0;
	MM = 0; 
	SS = 0;
	FS = 0;

	this->orientationClient = orientationClient;

	startTimer(50);
}

void Transport::updateOffset(int hh, int mm, int ss, int fs) {
	if ((hh != HH) || (mm != MM) || (ss != SS) || (fs != FS)) {
		HH = hh;
		MM = mm;
		SS = ss;
		FS = fs;

		/*
		juce::OSCMessage m = juce::OSCMessage("/offset");
		m.addInt32(HH);
		m.addInt32(MM);
		m.addInt32(SS);
		m.addInt32(FS);
		sender.send(m);
		*/

		transportOffset = hh * 1000 * 60 * 60 + mm * 1000 * 60 + ss * 1000 + fs;
	}
}

void Transport::sendData() {

	if (processor != nullptr)
	{
		//m.addFloat32((float) lastPosInfo.timeInSeconds); // << previous way
		orientationClient->command_setPlayerPositionInSeconds(correctTimeInSeconds + 1.0 * transportOffset / 1000);
		orientationClient->command_setPlayerIsPlaying(true);// lastPosInfo.isPlaying);
		reqResult = "Sent HH = " + String(HH) + " ; MM = " + String(MM) + ", packet #" + String(packetsSent);
		lastSentPositionValue = lastPosInfo.timeInSeconds;

		packetsSent++;
	}
}

void Transport::timerCallback() {
	updateCurrentTimeInfoFromHost();
	sendData();
}

void Transport::update()
{
	updateCurrentTimeInfoFromHost();

}

void Transport::setProcessor(AudioProcessor* processor)
{
	this->processor = processor;
}

void Transport::updateCurrentTimeInfoFromHost()
{
	if (processor != nullptr)
	{
		if (AudioPlayHead* ph = processor->getPlayHead())
		{
			AudioPlayHead::CurrentPositionInfo newTime;
			processor->getSampleRate();

			if (ph->getCurrentPosition(newTime))
			{
				lastPosInfo = newTime;  // Successfully got the current time from the host..
				correctTimeInSeconds = (lastPosInfo.ppqPosition / newTime.bpm) * 60;
                correctTimeInSeconds = lastPosInfo.timeInSeconds;
				return;
			}
		}

		// If the host fails to provide the current time, we'll just reset our copy to a default..
		lastPosInfo.resetToDefault();
		reqResult = "reverted to default";
	}
}

void Transport::showConnectionErrorMessage(String msg) {
	AlertWindow::showMessageBoxAsync(
		AlertWindow::WarningIcon,
		"Connection error",
		msg,
		"OK");
}
