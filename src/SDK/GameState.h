#pragma once
#include <cstdint>

class GameState {
public:
    static GameState& Get();
    void Initialize(uintptr_t moduleBase);
    void Update();

    // Public fields - dibaca dari memory game
    int Coin;
    int HP;
    int Kill;
    int Wave;
    int MaxWave;
    float Timer;
    uintptr_t GetModuleBase() const { return m_moduleBase; }

private:
    GameState() = default;
    uintptr_t m_moduleBase;
    uintptr_t m_basePointer;
};