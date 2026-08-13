#include "MonitorHostModePolicy.h"

#include <iostream>

namespace {

int failures = 0;

#define CHECK(condition)                                                           \
    do {                                                                           \
        if (!(condition)) {                                                        \
            ++failures;                                                            \
            std::cout << "FAILED: " #condition " (" << __FILE__ << ":"          \
                      << __LINE__ << ")" << std::endl;                             \
        }                                                                          \
    } while (false)

} // namespace

int main()
{
    using Mach1::MonitorHostModePolicy::AudioSource;
    using Mach1::MonitorHostModePolicy::selectAudioSource;

    // Native multichannel processing always wins when the host bus can carry
    // the selected decode format, even if a live helper MixBus also exists.
    CHECK(selectAudioSource(4, 4, false, false) == AudioSource::HostMultichannel);
    CHECK(selectAudioSource(8, 8, true, true) == AudioSource::HostMultichannel);
    CHECK(selectAudioSource(14, 14, true, true) == AudioSource::HostMultichannel);
    CHECK(selectAudioSource(14, 8, true, true) == AudioSource::HostMultichannel);

    // Narrow hosts use the helper only with both sides of the handshake.
    CHECK(selectAudioSource(2, 8, true, true) == AudioSource::ExternalMixBus);
    CHECK(selectAudioSource(2, 14, true, true) == AudioSource::ExternalMixBus);
    CHECK(selectAudioSource(1, 4, true, true) == AudioSource::ExternalMixBus);

    // A stale heartbeat or disconnected reader must never hijack the host bus
    // or pass narrow, unprocessed input through as decoded audio.
    CHECK(selectAudioSource(2, 8, false, true) == AudioSource::Silence);
    CHECK(selectAudioSource(2, 8, true, false) == AudioSource::Silence);
    CHECK(selectAudioSource(2, 8, false, false) == AudioSource::Silence);

    if (failures == 0) {
        std::cout << "All M1-Monitor host-mode policy tests passed" << std::endl;
        return 0;
    }

    std::cout << failures << " check(s) failed" << std::endl;
    return 1;
}
