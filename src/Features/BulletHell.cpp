#include "pch.h"
#include "BulletHell.h"
#include "SDK/GameState.h"
#include "Core/Logger/Logger.h"

static bool g_bulletHellActive = false;

void BulletHell::Toggle()
{
    g_bulletHellActive = !g_bulletHellActive;
    char log[128];
    sprintf_s(log, sizeof(log), "[BulletHell] %s", g_bulletHellActive ? "ACTIVE" : "INACTIVE");
    Logger::Log(log);
}

bool BulletHell::IsActive()
{
    return g_bulletHellActive;
}

void BulletHell::TriggerBarrage(void* originalTarget, float damage, int arg2, int arg3, float vx, float vy, float vz)
{
    if (!g_bulletHellActive) return;

    __try {
        uintptr_t base = GameState::Get().GetModuleBase();
        if (!base) return;

        uintptr_t gm = *(uintptr_t*)(base + 0x375860);
        if (!gm) return;

        uintptr_t level = *(uintptr_t*)(gm + 0x171c);
        if (!level) return;

        uintptr_t actorsBegin = *(uintptr_t*)(level + 0x184);
        uintptr_t actorsEnd   = *(uintptr_t*)(level + 0x188);
        if (!actorsBegin || !actorsEnd || actorsBegin >= actorsEnd) return;

        typedef bool(__thiscall* tTakeDamage)(void* thisPtr, float damage, int arg2, int arg3, float vx, float vy, float vz);
        tTakeDamage TakeDamage = (tTakeDamage)(base + 0x157B80);

        for (uintptr_t p = actorsBegin; p < actorsEnd; p += 4) {
            uintptr_t actor = *(uintptr_t*)p;
            if (!actor || *(int*)(actor + 0x140) != 4) continue; // Creeps only

            uintptr_t hc = *(uintptr_t*)(actor + 0x168); // HealthComponent
            if (!hc || (void*)hc == originalTarget) continue;

            float curHp = *(float*)(hc + 0x24);
            if (curHp <= 0.0f) continue;

            // Omnidirectional danmaku: distribute damage radially across all active creeps
            TakeDamage((void*)hc, damage, arg2, arg3, vx, vy, vz);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}
