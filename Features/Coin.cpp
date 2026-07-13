#include "pch.h"
#include "Coin.h"
#include "SDK/GameState.h"
#include "Core/Logger/Logger.h"

uintptr_t Coin::m_baseMenu = 0;
uintptr_t Coin::m_offsetMenu = 0xEC;
uintptr_t Coin::m_baseGame = 0;
uintptr_t Coin::m_offsetGame = 0x1778;

void Coin::Initialize(uintptr_t moduleBase) {
    m_baseMenu = moduleBase + 0x375874;
    m_baseGame = moduleBase + 0x375860;
    Logger::Log("[Coin] Initialized");
}

void Coin::AddMainMenu(int amount) {
    __try {
        uintptr_t ptrValue = *(uintptr_t*)(m_baseMenu);
        uintptr_t finalAddr = ptrValue + m_offsetMenu;
        *(int*)(finalAddr) += amount;
        GameState::Get().Coin += amount;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Log("[Coin] AddMainMenu failed!");
    }
}

void Coin::AddInGame(int amount) {
    __try {
        uintptr_t ptrValue = *(uintptr_t*)(m_baseGame);
        uintptr_t finalAddr = ptrValue + m_offsetGame;
        *(int*)(finalAddr) += amount;
        GameState::Get().Coin += amount;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Logger::Log("[Coin] AddInGame failed!");
    }
}