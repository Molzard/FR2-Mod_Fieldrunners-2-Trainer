#pragma once
#include <cstdint>

class AutoUpgrade {
public:
    static void Toggle();
    static bool IsActive();
    static void Update(uintptr_t moduleBase);
};
