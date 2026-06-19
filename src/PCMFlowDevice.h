#ifndef PCMFLOWDEVICE_H
#define PCMFLOWDEVICE_H

// Umbrella header for PCMFlowDevice.
//
// This header intentionally avoids board-specific dependencies. Include
// PCMFlowDeviceM5.h when using helpers backed by M5Unified.

#include <stdint.h>

#include "pcmflowdevice_version.h"

namespace pcmflowdevice
{
struct PlaybackProfile
{
    uint16_t initialPrebufferMs;
    uint16_t chunkMs;
};

inline PlaybackProfile lowLatencyProfile() { return {40, 20}; }
inline PlaybackProfile balancedProfile() { return {40, 40}; }
inline PlaybackProfile stableProfile() { return {80, 40}; }
} // namespace pcmflowdevice

#endif // PCMFLOWDEVICE_H
