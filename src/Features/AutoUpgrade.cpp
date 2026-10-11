#include "pch.h"
#include "AutoUpgrade.h"
#include "SDK/GameState.h"
#include "Core/Logger/Logger.h"

static bool g_autoUpgradeActive = false;
static DWORD g_lastUpgradeTick = 0;

void AutoUpgrade::Toggle()
{
    g_autoUpgradeActive = !g_autoUpgradeActive;
    char log[128];
    sprintf_s(log, sizeof(log), "[AutoUpgrade] %s", g_autoUpgradeActive ? "ACTIVE" : "INACTIVE");
    Logger::Log(log);
}

bool AutoUpgrade::IsActive()
{
    return g_autoUpgradeActive;
}

void AutoUpgrade::Update(uintptr_t moduleBase)
{
    if (!g_autoUpgradeActive) return;

    DWORD now = GetTickCount();
    if (now - g_lastUpgradeTick < 200) return; // run 5 times a second
    g_lastUpgradeTick = now;

    __try {
        uintptr_t base = moduleBase ? moduleBase : GameState::Get().GetModuleBase();
        if (!base) return;

        uintptr_t gm = *(uintptr_t*)(base + 0x375860);
        if (!gm) return;

        uintptr_t level = *(uintptr_t*)(gm + 0x171c);
        if (!level) return;

        uintptr_t gridMgr = *(uintptr_t*)(level + 0x1a8);
        if (!gridMgr) return;

        int width = *(int*)(gridMgr + 0x10);
        int height = *(int*)(gridMgr + 0x14);
        uintptr_t cells = *(uintptr_t*)(gridMgr + 0xC);
        if (!cells || width <= 0 || height <= 0 || width > 100 || height > 100) return;

        int totalCells = width * height * 2; // Cover tile layers

        typedef void(__thiscall* tUpgrade)(void* thisPtr);
        tUpgrade Upgrade = (tUpgrade)(base + 0x13D130);

        for (int i = 0; i < totalCells; i++) {
            uintptr_t cell = cells + (i * 60);
            uintptr_t actor = *(uintptr_t*)(cell + 0x10);
            if (!actor) continue;

            if (*(int*)(actor + 0x140) == 8) { // Tower
                uintptr_t towerComp = *(uintptr_t*)(actor + 0x160);
                if (!towerComp) continue;

                int curLevel = *(int*)(towerComp + 0x4C);
                if (curLevel < 2) {
                    int oldMoney = *(int*)(gm + 0x1778);
                    while (*(int*)(towerComp + 0x4C) < 2) {
                        Upgrade((void*)towerComp);
                    }
                    *(int*)(gm + 0x1778) = oldMoney; // Free upgrade
                    Logger::Log("[AutoUpgrade] Upgraded tower to Max Level (L3)!");
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Log("[AutoUpgrade] Error in update");
    }
}
