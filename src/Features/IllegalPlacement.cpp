#include "pch.h"
#include "IllegalPlacement.h"
#include "MinHook.h"
#include "Core/Logger/Logger.h"

typedef bool(__thiscall* tCanBuildAt)(void* thisPtr, void* actor, void* worldCoord);
static tCanBuildAt oCanBuildAt = nullptr;

static uintptr_t g_targetFunc = 0;
static bool g_illegalPlacementActive = false;

static bool __fastcall hkCanBuildAt(void* thisPtr, void* edx, void* actor, void* worldCoord)
{
    if (g_illegalPlacementActive) {
        return true; // Bypass pathfinding, red tiles & spawn zones!
    }
    return oCanBuildAt(thisPtr, actor, worldCoord);
}

bool IllegalPlacement::Initialize(uintptr_t moduleBase)
{
    if (!moduleBase) return false;

    g_targetFunc = moduleBase + 0x114F40; // CanBuildAt

    MH_Initialize();

    if (MH_CreateHook((void*)g_targetFunc, &hkCanBuildAt, (void**)&oCanBuildAt) != MH_OK) {
        Logger::Log("[IllegalPlacement] Hook create failed!");
        return false;
    }

    if (MH_EnableHook((void*)g_targetFunc) != MH_OK) {
        Logger::Log("[IllegalPlacement] Hook enable failed!");
        return false;
    }

    Logger::Log("[IllegalPlacement] Hook enabled at 0x114F40");
    return true;
}

void IllegalPlacement::Toggle()
{
    g_illegalPlacementActive = !g_illegalPlacementActive;
    char log[128];
    sprintf_s(log, sizeof(log), "[IllegalPlacement] %s", g_illegalPlacementActive ? "ACTIVE" : "INACTIVE");
    Logger::Log(log);
}

bool IllegalPlacement::IsActive()
{
    return g_illegalPlacementActive;
}

void IllegalPlacement::Shutdown()
{
    if (g_targetFunc) {
        MH_DisableHook((void*)g_targetFunc);
        MH_RemoveHook((void*)g_targetFunc);
        g_targetFunc = 0;
    }
    g_illegalPlacementActive = false;
}
