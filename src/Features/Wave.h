#pragma once
#include <cstdint>

class WaveFeature {
public:
    static void Initialize(uintptr_t moduleBase);
    static void SkipWave();
    static void InstantFinish();
};
