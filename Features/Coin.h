#pragma once
#include <cstdint>

class Coin {
public:
    static void AddMainMenu(int amount);
    static void AddInGame(int amount);
    static void Initialize(uintptr_t moduleBase);

private:
    static uintptr_t m_baseMenu;
    static uintptr_t m_offsetMenu;
    static uintptr_t m_baseGame;
    static uintptr_t m_offsetGame;
};