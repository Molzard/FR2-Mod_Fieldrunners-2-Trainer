#include "pch.h"
#include "GameState.h"
#include "Core/Logger/Logger.h"

GameState& GameState::Get() {
    static GameState instance;
    return instance;
}

void GameState::Initialize(uintptr_t moduleBase) {
    m_moduleBase = moduleBase;

    // Baca base pointer (Fieldrunners2.dll + 0x375860)
    uintptr_t basePtr = *(uintptr_t*)(m_moduleBase + 0x375860);
    m_basePointer = basePtr;

    char log[256];
    sprintf_s(log, sizeof(log), "[SDK] GameState init: basePtr=0x%X", basePtr);
    Logger::Log(log);
}

void GameState::Update() {
    if (!m_basePointer) return;

    // Baca semua nilai dari memory
    Coin = *(int*)(m_basePointer + 0x1778);
    HP = *(int*)(m_basePointer + 0x177C);
    Kill = *(int*)(m_basePointer + 0x1828);
    Wave = *(int*)(m_basePointer + 0x187C);
    MaxWave = *(int*)(m_basePointer + 0x1874);
    Timer = *(float*)(m_basePointer + 0x1984);
}