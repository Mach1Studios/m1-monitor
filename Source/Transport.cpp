
#include "Transport.h"

//==============================================================================
Transport::Transport(M1OrientationClient* orientationClient)
{
	HH = 0;
	MM = 0; 
	SS = 0;
	FS = 0;

	this->m_orientation_client = orientationClient;

    startTimer(50);
}

void Transport::sendData() {

    if (m_orientation_client == nullptr) return;

    auto play_head_position = m_correct_time_in_seconds + 1.0 * m_transport_offset / 1000;

    m_orientation_client->command_setPlayerPositionInSeconds(play_head_position);
    m_orientation_client->command_setPlayerIsPlaying(getIsPlaying());
}

void Transport::setOffset(int hh, int mm, int ss, int fs) {
    if ((hh == HH) && (mm == MM) && (ss == SS) && (fs == FS)) {
        return;
    }

    HH = hh;
    MM = mm;
    SS = ss;
    FS = fs;

    m_transport_offset = hh * 1000 * 60 * 60 + mm * 1000 * 60 + ss * 1000 + fs;
}

double Transport::getTimeInSeconds() const {
    return m_correct_time_in_seconds;
}

void Transport::setTimeInSeconds(double time_in_seconds) {
    m_correct_time_in_seconds = time_in_seconds;
}

bool Transport::getIsPlaying() const {
    return m_is_playing;
}

void Transport::setIsPlaying(bool is_playing) {
    m_is_playing = is_playing;
}

void Transport::timerCallback() {
    sendData();
}
