#ifndef TYPESFORDATAEXCHANGE_H
#define TYPESFORDATAEXCHANGE_H

#include "Mach1Decode.h"

struct MixerSettings {
    /// This object contains:
    /// - `Mach1DecodeAlgoType`
    Mach1Decode m1Decode;
    
	int monitor_input_channel_count;
	int monitor_output_channel_count;
	float yaw;
	float pitch;
	float roll;
    int monitor_mode;
    
    bool yawActive, pitchActive, rollActive = true;
};

#endif
