
#include "Transport.h"

//==============================================================================
Transport::Transport(M1OrientationOSCClient* osc_client)
{
    if (!sender.connect("127.0.0.1", osc_client->getServerPort())) {
		showConnectionErrorMessage("Error: could not connect to UDP port "+std::to_string(osc_client->getServerPort())+".");
    }

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
		juce::OSCMessage m = juce::OSCMessage("/offset");
		m.addInt32(HH);
		m.addInt32(MM);
		m.addInt32(SS);
		m.addInt32(FS);
		sender.send(m);
	}
}

void Transport::sendDataViaOsc() {

	if (processor != nullptr)
	{
		juce::OSCMessage m = juce::OSCMessage("/transport");
		//m.addFloat32((float) lastPosInfo.timeInSeconds); // << previous way
		m.addFloat32(correctTimeInSeconds); // << testing new way
		m.addInt32((lastPosInfo.isPlaying ? 1 : 0));
		reqResult = "Sent HH = " + String(HH) + " ; MM = " + String(MM) + ", packet #" + String(packetsSent);
		sender.send(m);
		lastSentPositionValue = lastPosInfo.timeInSeconds;

		packetsSent++;
	}
}

void Transport::timerCallback() {
	updateCurrentTimeInfoFromHost();
	sendDataViaOsc();
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
