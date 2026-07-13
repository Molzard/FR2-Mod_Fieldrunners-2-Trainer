#pragma once

// ============================================================
// Menu Item Types
// ============================================================
#define MAX_ITEMS 10

// Item types
struct MenuItem {
    const char* name;
    bool isParent;
    int parentIndex;
    int actionType;
    int value;
};

// ============================================================
// Menu Manager Class
// ============================================================
class MenuManager {
public:
    static void Build();
    static void UpdateVisible();
    static void HandleNavigation();
    static void Render(class GLFont& font, int frameCount);

    static bool IsVisible() { return m_visible; }
    static int GetVisibleCount() { return m_visibleCount; }
    static MenuItem* GetItems() { return m_items; }
    static int GetItemLevel(int index) { return m_itemLevel[index]; }
    static int GetSelectedIndex() { return m_menuIndex; }
    static bool IsExpanded(int index) { return m_expanded[index]; }
    static int GetRealIndex(int visibleIndex) { return m_visibleItems[visibleIndex]; }

private:
    static MenuItem m_items[MAX_ITEMS];
    static int m_itemLevel[MAX_ITEMS];
    static int m_totalItems;
    static int m_menuIndex;
    static bool m_expanded[MAX_ITEMS];
    static int m_visibleItems[MAX_ITEMS];
    static int m_visibleCount;
    static bool m_visible;
};