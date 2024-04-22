
#include "Transport.h"

//==============================================================================
Transport::Transport(M1OrientationClient* orientationClient) : m_orientation_client(orientationClient)
{
    startTimer(50);
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

void Transport::sendData() {

    if (m_orientation_client == nullptr) return;

    m_transport_offset = HH * 1000 * 60 * 60 + MM * 1000 * 60 + SS * 1000 + FS;
    auto play_head_position = m_correct_time_in_seconds + m_transport_offset / 1000;

    m_orientation_client->command_setPlayerPositionInSeconds(play_head_position);
    m_orientation_client->command_setPlayerIsPlaying(getIsPlaying());
}

void Transport::timerCallback() {
    sendData();
}