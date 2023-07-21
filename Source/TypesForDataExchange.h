#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include "Mach1Decode.h"

struct MixerSettings {
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode m1Decode;
    
	float yaw = 0;
	float pitch = 0;
	float roll = 0;
    int monitor_mode = 0;
    int osc_port = 9898;
    
    bool yawActive = true, pitchActive = true, rollActive = true;
};

#endif
