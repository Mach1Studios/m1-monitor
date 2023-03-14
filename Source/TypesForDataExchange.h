#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include "Mach1Decode.h"

struct MixerSettings {
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode m1Decode;
    
	float yaw;
	float pitch;
	float roll;
    int monitor_mode;
    
    bool yawActive, pitchActive, rollActive = true;
};

#endif
