#pragma once
#include <cstdint>

class BulletHell {
public:
    static void Toggle();
    static bool IsActive();
    static void TriggerBarrage(void* originalTarget, float damage, int arg2, int arg3, float vx, float vy, float vz);
};
