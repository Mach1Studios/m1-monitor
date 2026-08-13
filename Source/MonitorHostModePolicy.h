#pragma once

namespace Mach1 {
namespace MonitorHostModePolicy {

enum class AudioSource
{
    HostMultichannel,
    ExternalMixBus,
    Silence
};

/**
 * Selects the monitor's audio source without touching host or IPC state.
 *
 * A host bus wide enough for the selected decode format always wins, even if
 * a helper heartbeat and MixBus happen to exist. The external MixBus is only
 * valid for a narrow mono/stereo host with a fresh streaming heartbeat and a
 * connected reader. Otherwise silence is safer than passing unprocessed input.
 */
constexpr AudioSource selectAudioSource(int hostInputChannels,
                                        int decodeChannels,
                                        bool streamingHeartbeatFresh,
                                        bool mixBusReaderConnected)
{
    if (hostInputChannels >= decodeChannels)
        return AudioSource::HostMultichannel;

    if (streamingHeartbeatFresh && mixBusReaderConnected)
        return AudioSource::ExternalMixBus;

    return AudioSource::Silence;
}

} // namespace MonitorHostModePolicy
} // namespace Mach1
