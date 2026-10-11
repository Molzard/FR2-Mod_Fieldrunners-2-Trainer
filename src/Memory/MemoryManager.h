#pragma once

#include <cstdint>

class MemoryManager
{
private:
    uintptr_t m_BaseAddress = 0;

public:
    MemoryManager();

    bool Initialize();
    uintptr_t GetBaseAddress() const;
};