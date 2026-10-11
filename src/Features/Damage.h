#pragma once
#include <cstdint>

class Damage {
public:
    static bool Initialize(uintptr_t moduleBase);
    static void Toggle();
    static bool IsActive();
    static float GetMultiplier();
    static const char* GetStatusText();
    static void Shutdown();
};
