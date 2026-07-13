#include "pch.h"
#include "MenuManager.h"
#include "Features/Coin.h"
#include "Features/Health.h"
#include "Core/Logger/Logger.h"

// Static member definitions
MenuItem MenuManager::m_items[MAX_ITEMS];
int MenuManager::m_itemLevel[MAX_ITEMS]; // 0 = root, 1 = child

// Ganti menuIndex & items:
int MenuManager::m_menuIndex = 0;
int MenuManager::m_totalItems = 0;
bool MenuManager::m_expanded[MAX_ITEMS] = {false};

// Hitung items yang visible (expand/collapse)
int MenuManager::m_visibleItems[MAX_ITEMS];
int MenuManager::m_visibleCount = 0;

bool MenuManager::m_visible = true;

void MenuManager::Build()
{
    m_totalItems = 0;

    // === Parent 1: Main Menu ===
    m_items[m_totalItems].name = "Main Menu";
    m_items[m_totalItems].isParent = true;
    m_items[m_totalItems].parentIndex = -1;
    m_items[m_totalItems].actionType = 0;
    m_itemLevel[m_totalItems] = 0;
    m_totalItems++;

    // Child of Main Menu
    m_items[m_totalItems].name = "Add Coin (+1000)";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 0;
    m_items[m_totalItems].actionType = 1;
    m_items[m_totalItems].value = 1000;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // === Parent 2: In-Game ===
    m_items[m_totalItems].name = "In-Game";
    m_items[m_totalItems].isParent = true;
    m_items[m_totalItems].parentIndex = -1;
    m_items[m_totalItems].actionType = 0;
    m_itemLevel[m_totalItems] = 0;
    m_totalItems++;

    // Child of In-Game
    m_items[m_totalItems].name = "Add Money (+5000)";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 2;
    m_items[m_totalItems].value = 5000;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // Child of In-Game
    m_items[m_totalItems].name = "Freeze Player Health";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 3;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;
}

void MenuManager::UpdateVisible()
{
    m_visibleCount = 0;
    bool parentExpanded[10] = { false };

    for (int i = 0; i < m_totalItems; i++) {
        if (m_items[i].parentIndex == -1) {
            // Root item - always visible
            m_visibleItems[m_visibleCount++] = i;
            parentExpanded[i] = m_expanded[i];
        }
        else {
            // Child - visible only if parent expanded
            int parent = m_items[i].parentIndex;
            if (parentExpanded[parent]) {
                m_visibleItems[m_visibleCount++] = i;
            }
        }
    }
}

void MenuManager::HandleNavigation()
{
    UpdateVisible();

    static DWORD lastKey = 0;
    DWORD now = GetTickCount();
    if (now - lastKey < 200) return;
    if (m_visibleCount == 0) return;

    if (GetAsyncKeyState('I') & 0x8000) {
        m_menuIndex = (m_menuIndex - 1 + m_visibleCount) % m_visibleCount;
        lastKey = now;
    }
    if (GetAsyncKeyState('K') & 0x8000) {
        m_menuIndex = (m_menuIndex + 1) % m_visibleCount;
        lastKey = now;
    }
    if (GetAsyncKeyState('L') & 0x8000) {
        m_visible = !m_visible;
        lastKey = now;
    }
    if (GetAsyncKeyState('J') & 0x8000) {
        int realIndex = m_visibleItems[m_menuIndex];

        __try {
            if (m_items[realIndex].isParent) {
                m_expanded[realIndex] = !m_expanded[realIndex];
            }
            else {
                switch (m_items[realIndex].actionType) {
                case 1: {
                    Coin::AddMainMenu(m_items[realIndex].value);
                    break;
                }
                case 2: {
                    Coin::AddInGame(m_items[realIndex].value);
                    break;
                }
                case 3:
                    Health::Toggle();
                    break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::Log("[ERR] Action failed!");
        }
        lastKey = now;
    }
}