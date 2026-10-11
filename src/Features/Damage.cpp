#include "pch.h"
#include "Damage.h"
#include "BulletHell.h"
#include "MinHook.h"
#include "Core/Logger/Logger.h"

typedef bool(__thiscall* tTakeDamage)(void* thisPtr, float damage, int arg2, int arg3, float vx, float vy, float vz);
static tTakeDamage oTakeDamage = nullptr;

static uintptr_t g_targetFunc = 0;
static int g_multiplierState = 0; // 0=OFF, 1=2x, 2=5x, 3=10x, 4=100x
static const float g_multipliers[] = { 1.0f, 2.0f, 5.0f, 10.0f, 100.0f };
static const char* g_statusTexts[] = {
    "Damage Multiplier: OFF",
    "Damage Multiplier: 2x",
    "Damage Multiplier: 5x",
    "Damage Multiplier: 10x",
    "Damage Multiplier: 100x"
};

static bool g_inBarrage = false;

// __fastcall: ecx = thisPtr, edx = dummy, stack matches caller __thiscall frame
static bool __fastcall hkTakeDamage(void* thisPtr, void* edx, float damage, int arg2, int arg3, float vx, float vy, float vz)
{
    if (g_multiplierState > 0 && damage > 0.0f) {
        damage *= g_multipliers[g_multiplierState];
    }

    if (!g_inBarrage && damage > 0.0f && BulletHell::IsActive()) {
        g_inBarrage = true;
        BulletHell::TriggerBarrage(thisPtr, damage, arg2, arg3, vx, vy, vz);
        g_inBarrage = false;
    }

    return oTakeDamage(thisPtr, damage, arg2, arg3, vx, vy, vz);
}

bool Damage::Initialize(uintptr_t moduleBase)
{
    if (!moduleBase) return false;

    g_targetFunc = moduleBase + 0x157B80; // HealthComponent::TakeDamage

    MH_Initialize();

    if (MH_CreateHook((void*)g_targetFunc, &hkTakeDamage, (void**)&oTakeDamage) != MH_OK) {
        Logger::Log("[Damage] Failed to create hook!");
        return false;
    }

    if (MH_EnableHook((void*)g_targetFunc) != MH_OK) {
        Logger::Log("[Damage] Failed to enable hook!");
        return false;
    }

    Logger::Log("[Damage] Hook enabled at 0x157B80");
    return true;
}

void Damage::Toggle()
{
    g_multiplierState = (g_multiplierState + 1) % 5;
    char log[128];
    sprintf_s(log, sizeof(log), "[Damage] Switched to: %s", g_statusTexts[g_multiplierState]);
    Logger::Log(log);
}

bool Damage::IsActive()
{
    return g_multiplierState > 0;
}

float Damage::GetMultiplier()
{
    return g_multipliers[g_multiplierState];
}

const char* Damage::GetStatusText()
{
    return g_statusTexts[g_multiplierState];
}

void Damage::Shutdown()
{
    if (g_targetFunc) {
        MH_DisableHook((void*)g_targetFunc);
        MH_RemoveHook((void*)g_targetFunc);
        g_targetFunc = 0;
    }
    g_multiplierState = 0;
}
