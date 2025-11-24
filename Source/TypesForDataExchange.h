#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include <Mach1Decode.h>

struct MixerSettings
{
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode<float> m1Decode;
    Mach1Decode<float> m1Decode2;

    float yaw = 0; // degree range   -180->180
    float pitch = 0; // degree range -90->90
    float roll = 0; // degree range -90->90
    float fieldOfHearing = 45.0f; // degree range 0->180, default ±45 degrees field of hearing
    float fieldOfHearing2 = 0.0f; // degree range 0->180, default 0 degrees field of hearing
    float fohRatio = 1.0f; // range 0->1, default 1.0 (100% main signal)
    int monitor_mode = 0;

    bool yawActive = true, pitchActive = true, rollActive = true;
};

#endif
