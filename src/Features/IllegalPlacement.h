#pragma once
#include <cstdint>

class IllegalPlacement {
public:
    static bool Initialize(uintptr_t moduleBase);
    static void Toggle();
    static bool IsActive();
    static void Shutdown();
};
