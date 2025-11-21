#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include <Mach1Decode.h>

struct MixerSettings
{
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode<float> m1Decode;

    float yaw = 0; // degree range   -180->180
    float pitch = 0; // degree range -90->90
    float roll = 0; // degree range -90->90
    float fieldOfHearing = 45.0f; // degree range 0->180, default ±45 degrees field of hearing
    int monitor_mode = 0;

    bool yawActive = true, pitchActive = true, rollActive = true;
};

#endif
