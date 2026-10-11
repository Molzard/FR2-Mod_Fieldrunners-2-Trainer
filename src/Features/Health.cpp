#include "pch.h"
#include "Health.h"
#include "SDK/GameState.h"
#include "Core/Logger/Logger.h"

static bool g_freezeHealth = false;
static bool g_freezeHealthActive = false;
static HANDLE g_hThread = nullptr;

DWORD WINAPI FreezeHealthThread(LPVOID) {
    while (g_freezeHealthActive) {
        if (g_freezeHealth) {
            __try {
                // Ini masih pakai cara lama, nanti diganti SDK
                uintptr_t base = GameState::Get().GetModuleBase();
                uintptr_t ptrValue = *(uintptr_t*)(base + 0x375860);
                *(int*)(ptrValue + 0x177C) = 100;
                GameState::Get().HP = 100;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                Logger::Log("[Health] Write failed!");
            }
        }
        Sleep(50);
    }
    return 0;
}

void Health::Toggle() {
    g_freezeHealth = !g_freezeHealth;
    if (g_freezeHealth && !g_freezeHealthActive) {
        g_freezeHealthActive = true;
        g_hThread = CreateThread(nullptr, 0, FreezeHealthThread, nullptr, 0, nullptr);
    }
}

bool Health::IsActive() {
    return g_freezeHealth;
}

void Health::Shutdown() {
    g_freezeHealthActive = false;
    g_freezeHealth = false;
    if (g_hThread) {
        WaitForSingleObject(g_hThread, 200);
        CloseHandle(g_hThread);
        g_hThread = nullptr;
    }
}