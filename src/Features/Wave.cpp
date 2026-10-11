#include "pch.h"
#include "Wave.h"
#include "SDK/GameState.h"
#include "Core/Logger/Logger.h"

static uintptr_t g_moduleBase = 0;

void WaveFeature::Initialize(uintptr_t moduleBase)
{
    g_moduleBase = moduleBase;
    Logger::Log("[Wave] Initialized");
}

void WaveFeature::SkipWave()
{
    __try {
        uintptr_t base = g_moduleBase ? g_moduleBase : GameState::Get().GetModuleBase();
        if (!base) return;

        uintptr_t wm = *(uintptr_t*)(base + 0x3758A0);
        if (!wm) {
            Logger::Log("[Wave] WaveManager null");
            return;
        }

        int currentIdx = *(int*)(wm + 0x60);
        uintptr_t* waves = *(uintptr_t**)(wm + 0xC);
        uintptr_t* wavesEnd = *(uintptr_t**)(wm + 0x10);

        if (!waves || !wavesEnd || waves >= wavesEnd) {
            Logger::Log("[Wave] Waves vector invalid");
            return;
        }

        int totalWaves = (int)(wavesEnd - waves);
        int nextIdx = currentIdx + 1;

        if (nextIdx < totalWaves) {
            uintptr_t waveObj = waves[nextIdx];
            if (waveObj) {
                typedef void(__thiscall* tWaveStart)(void* thisPtr);
                void** vtable = *(void***)waveObj;
                if (vtable && vtable[2]) {
                    tWaveStart WaveStart = (tWaveStart)vtable[2];
                    WaveStart((void*)waveObj);
                }

                *(int*)(wm + 0x60) = nextIdx;

                uintptr_t gm = *(uintptr_t*)(base + 0x375860);
                if (gm) {
                    *(int*)(gm + 0x187C) = nextIdx + 1;
                    GameState::Get().Wave = nextIdx + 1;
                }

                char log[128];
                sprintf_s(log, sizeof(log), "[Wave] Skip -> Wave %d / %d", nextIdx + 1, totalWaves);
                Logger::Log(log);
            }
        }
        else {
            Logger::Log("[Wave] Last wave reached, finishing map...");
            InstantFinish();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Log("[Wave] SkipWave error");
    }
}

void WaveFeature::InstantFinish()
{
    __try {
        uintptr_t base = g_moduleBase ? g_moduleBase : GameState::Get().GetModuleBase();
        if (!base) return;

        uintptr_t gm = *(uintptr_t*)(base + 0x375860);
        if (!gm) {
            Logger::Log("[Wave] GameManager null");
            return;
        }

        int maxWave = *(int*)(gm + 0x1874);
        *(int*)(gm + 0x187C) = maxWave;
        GameState::Get().Wave = maxWave;

        uintptr_t wm = *(uintptr_t*)(base + 0x3758A0);
        if (wm) {
            uintptr_t* waves = *(uintptr_t**)(wm + 0xC);
            uintptr_t* wavesEnd = *(uintptr_t**)(wm + 0x10);
            if (waves && wavesEnd && waves < wavesEnd) {
                int totalWaves = (int)(wavesEnd - waves);
                *(int*)(wm + 0x60) = totalWaves - 1;
            }
        }

        void* levelMgr = (void*)(gm + 0x1720);
        typedef void(__thiscall* tEndGame)(void* thisPtr, int victory);
        tEndGame EndGame = (tEndGame)(base + 0x14F0A0);
        EndGame(levelMgr, 1);

        Logger::Log("[Wave] Instant finish triggered (Victory)");
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Log("[Wave] InstantFinish error");
    }
}
