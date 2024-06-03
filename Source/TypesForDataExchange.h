#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include <Mach1Decode.h> 

struct MixerSettings {
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode m1Decode;
    
	float yaw = 0;   // degree range   0->360
	float pitch = 0; // degree range -90->90
	float roll = 0;  // degree range -90->90
    int monitor_mode = 0;
    
    bool yawActive = true, pitchActive = true, rollActive = true;
};

#endif
