#include "pch.h"
#include <iostream>
#include <Windows.h>

#include "MemoryManager.h"

MemoryManager::MemoryManager()
{
    std::cout << "[Memory] Constructor\n";

}

bool MemoryManager::Initialize()
{
    std::cout << "[Memory] Initialize\n";

    HMODULE hModule = GetModuleHandleA("Fieldrunners2.dll");

    if (hModule == nullptr)
    {
        std::cout << "[Memory] Failed to get module handle!\n";
        return false;
    }

    m_BaseAddress = reinterpret_cast<uintptr_t>(hModule);

    std::cout << "[Memory] Base Address : 0x"
        << std::hex
        << m_BaseAddress
        << '\n';

    return true;
}

uintptr_t MemoryManager::GetBaseAddress() const
{
    return m_BaseAddress;
}