#include "pch.h"
#include "MenuManager.h"
#include "Features/Coin.h"
#include "Features/Health.h"
#include "Features/Damage.h"
#include "Features/Wave.h"
#include "Features/AutoUpgrade.h"
#include "Features/IllegalPlacement.h"
#include "Features/BulletHell.h"
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

    // Child of In-Game: Damage Multiplier
    m_items[m_totalItems].name = Damage::GetStatusText();
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 4;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // Child of In-Game: Auto Upgrade
    m_items[m_totalItems].name = "Auto Upgrade (Max L3)";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 7;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // Child of In-Game: Illegal Grid Placement
    m_items[m_totalItems].name = "Illegal Grid Placement";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 8;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // Child of In-Game: Bullet Hell 360
    m_items[m_totalItems].name = "Bullet Hell 360 Barrage";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = 2;
    m_items[m_totalItems].actionType = 10;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // === Parent 3: Wave ===
    int waveParent = m_totalItems;
    m_items[m_totalItems].name = "Wave";
    m_items[m_totalItems].isParent = true;
    m_items[m_totalItems].parentIndex = -1;
    m_items[m_totalItems].actionType = 0;
    m_itemLevel[m_totalItems] = 0;
    m_totalItems++;

    // Child of Wave: Skip Wave
    m_items[m_totalItems].name = "Skip to Next Wave";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = waveParent;
    m_items[m_totalItems].actionType = 5;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;

    // Child of Wave: Instant Selesai
    m_items[m_totalItems].name = "Instant Selesai (Win)";
    m_items[m_totalItems].isParent = false;
    m_items[m_totalItems].parentIndex = waveParent;
    m_items[m_totalItems].actionType = 6;
    m_items[m_totalItems].value = 0;
    m_itemLevel[m_totalItems] = 1;
    m_totalItems++;
}

void MenuManager::UpdateVisible()
{
    m_visibleCount = 0;
    bool parentExpanded[MAX_ITEMS] = { false };

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
                case 4:
                    Damage::Toggle();
                    m_items[realIndex].name = Damage::GetStatusText();
                    break;
                case 5:
                    WaveFeature::SkipWave();
                    break;
                case 6:
                    WaveFeature::InstantFinish();
                    break;
                case 7:
                    AutoUpgrade::Toggle();
                    break;
                case 8:
                    IllegalPlacement::Toggle();
                    break;
                case 10:
                    BulletHell::Toggle();
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