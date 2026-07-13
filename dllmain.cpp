#include "pch.h"
#include <windows.h>
#include <psapi.h>
#include <string>
#include "MinHook.h"
#include <GL/gl.h>
#include <ctime>

#include "Core/Logger/Logger.h"
#include "SDK/GameState.h"
#include "Features/Health.h"
#include "Features/Coin.h"
#include "UI/MenuManager.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "libMinHook.x86.lib")
#pragma comment(lib, "opengl32.lib")

extern "C" {
    void* pOriginalSwap = nullptr;
    void* pOriginalPresent = nullptr;
    void* pOriginalLayer = nullptr;
}

int g_frameCount = 0;
bool g_inRender = false;
int swapCount = 0, presentCount = 0, layerCount = 0;

GLuint g_fontBase = 0;

// Context tracking
HGLRC g_lastContext = nullptr;
HDC   g_lastDC = nullptr;

uintptr_t g_moduleBase = 0;

void BuildFont(HDC hdc)
{
    if (g_fontBase) return;
    g_fontBase = glGenLists(128);
    HFONT font = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Consolas");
    HGDIOBJ oldFont = SelectObject(hdc, font);
    wglUseFontBitmaps(hdc, 0, 128, g_fontBase);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void DrawTextGL(float x, float y, const char* text, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    if (!g_fontBase) return;
    glColor4ub(r, g, b, a);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(g_fontBase);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void RenderMenuGL()
{
    HDC hdc = wglGetCurrentDC();
    HGLRC ctx = wglGetCurrentContext();
    if (!hdc || !ctx) return;

    if (ctx != g_lastContext || hdc != g_lastDC) {
        char log[256];
        sprintf_s(log, sizeof(log), "[CTX] Changed! old=%p new=%p", g_lastContext, ctx);
        Logger::Log(log);
        g_lastContext = ctx;
        g_lastDC = hdc;
        g_fontBase = 0;
    }

    if (!g_fontBase) BuildFont(hdc);

    // === SIMPAN SEMUA STATE ===
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1024, 768, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // === RESET SEMUA STATE UNTUK 2D RENDERING ===
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);

    int fontOffsetY = 14;

    // === PERHITUNGAN WAKTU OS REAL-TIME ===
    time_t rawtime;
    struct tm timeinfo;
    char timeBuffer[32];
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    // Format: HH:MM:SS (Contoh: 20:15:46)
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo);

    // === PERHITUNGAN TINGGI BACKGROUND DINAMIS ===
    MenuManager::UpdateVisible();
    int bgHeight = 55 + MenuManager::GetVisibleCount() * 30;
    if (bgHeight < 100) bgHeight = 100;

    int menuWidth = 430;
    int startX = 20;
    int startY = 20;

    // === 1. PERSONA 5 SLANTED BG EFFECT (SHADOW BACKGROUND) ===
    glColor4ub(10, 10, 10, 180);
    glBegin(GL_QUADS);
    glVertex2f(startX + 10, startY + 5);
    glVertex2f(menuWidth + 25, startY - 5);
    glVertex2f(menuWidth + 10, bgHeight + 45);
    glVertex2f(startX, bgHeight + 35);
    glEnd();

    // === 2. MAIN SOLID BG (HIGH CONTRAST BLACK) ===
    glColor4ub(20, 20, 23, 245);
    glBegin(GL_QUADS);
    glVertex2f(startX, startY);
    glVertex2f(menuWidth, startY);
    glVertex2f(menuWidth, bgHeight + 40);
    glVertex2f(startX, bgHeight + 40);
    glEnd();

    // === 3. PERSONA 5 ACCENT TOP HEADER BLOCK (THE SIGNATURE RED) ===
    glBegin(GL_QUADS);
    glColor4ub(220, 10, 40, 255);
    glVertex2f(startX, startY);
    glVertex2f(menuWidth, startY);
    glColor4ub(150, 0, 20, 255);
    glVertex2f(menuWidth, startY + 38);
    glVertex2f(startX, startY + 38);
    glEnd();

    // Header Stripe Accent (Garis dekoratif putih)
    glColor4ub(255, 255, 255, 200);
    glBegin(GL_LINES);
    glVertex2f(startX, startY + 38);
    glVertex2f(menuWidth, startY + 38);
    glEnd();

    // RENDER TITLE TEXT & WAKTU OS (Diletakkan di Header Merah)
    DrawTextGL(startX + 15, startY + 11 + fontOffsetY, ">> FIELDRUNNERS 2 LEGIT", 255, 255, 255, 255);

    // Tampilkan Jam di ujung kanan header merah dengan warna putih kontras
    DrawTextGL(menuWidth - 90, startY + 11 + fontOffsetY, timeBuffer, 255, 255, 255, 220);

    // === 4. LOOP RENDERING MENU ITEMS ===
    for (int i = 0; i < MenuManager::GetVisibleCount(); i++) {
        int realIndex = MenuManager::GetRealIndex(i);
        bool isSelected = (i == MenuManager::GetSelectedIndex());
        int indent = MenuManager::GetItemLevel(realIndex) * 24;

        int itemTop = startY + 45 + (i * 30);
        int itemBottom = itemTop + 26;
        int itemLeft = startX + 10 + indent;
        int itemRight = menuWidth - 10;

        int pulseAlpha = 220 + (int)(35.0f * sinf(g_frameCount * 0.15f));

        if (isSelected) {
            glBegin(GL_QUADS);
            glColor4ub(220, 10, 40, pulseAlpha);
            glVertex2f(itemLeft - 4, itemTop);
            glVertex2f(itemRight + 4, itemTop - 1);
            glColor4ub(180, 0, 25, pulseAlpha);
            glVertex2f(itemRight, itemBottom + 1);
            glVertex2f(itemLeft - 4, itemBottom);
            glEnd();

            glColor4ub(255, 255, 255, 255);
            glBegin(GL_QUADS);
            glVertex2f(itemLeft - 8, itemTop);
            glVertex2f(itemLeft - 5, itemTop);
            glVertex2f(itemLeft - 5, itemBottom);
            glVertex2f(itemLeft - 8, itemBottom);
            glEnd();
        }
        else {
            glColor4ub(35, 35, 40, 130);
            glBegin(GL_QUADS);
            glVertex2f(itemLeft, itemTop);
            glVertex2f(itemRight, itemTop);
            glVertex2f(itemRight, itemBottom);
            glVertex2f(itemLeft, itemBottom);
            glEnd();
        }

        char text[256];
        if (MenuManager::GetItems()[realIndex].isParent) {
            sprintf_s(text, "%s  %s", MenuManager::IsExpanded(realIndex) ? "▼" : "▶", MenuManager::GetItems()[realIndex].name);
        }
        else if (MenuManager::GetItems()[realIndex].actionType == 3) {
            sprintf_s(text, "%s: %s", MenuManager::GetItems()[realIndex].name,
                Health::IsActive() ? "ACTIVE" : "INACTIVE");
        }
        else {
            strcpy_s(text, MenuManager::GetItems()[realIndex].name);
        }

        GLubyte r, g, b;
        if (isSelected) {
            r = 255; g = 255; b = 255;
        }
        else {
            if (MenuManager::GetItems()[realIndex].isParent) {
                r = 220; g = 220; b = 225;
            }
            else {
                r = 160; g = 165; b = 175;
            }
        }

        DrawTextGL(itemLeft + 10, itemTop + 5 + fontOffsetY, text, r, g, b, 255);

        if (!MenuManager::GetItems()[realIndex].isParent && MenuManager::GetItems()[realIndex].actionType == 3 && Health::IsActive()) {
            int badgeLeft = itemRight - 75;
            glColor4ub(255, 255, 255, isSelected ? 255 : 180);
            glBegin(GL_LINE_LOOP);
            glVertex2f(badgeLeft, itemTop + 4);
            glVertex2f(itemRight - 5, itemTop + 4);
            glVertex2f(itemRight - 5, itemBottom - 4);
            glVertex2f(badgeLeft, itemBottom - 4);
            glEnd();
        }
    }

    // === 5. DYNAMIC FOOTER & CONTROLS SECTION ===
    int footerStartY = bgHeight + 3;

    glColor4ub(70, 70, 75, 180);
    glBegin(GL_LINES);
    glVertex2f(startX + 10, footerStartY);
    glVertex2f(menuWidth - 10, footerStartY);
    glEnd();

    // Petunjuk Navigasi
    DrawTextGL(startX + 12, footerStartY + 6 + fontOffsetY, "[I/K] NAVIGATE   [J] ACTION   [L] CLOSE", 140, 142, 150, 220);

    // Debug: GameState info
    GameState& gs = GameState::Get();
    char gsText[128];
    sprintf_s(gsText, "W:%d/%d K:%d C:%d HP:%d", gs.Wave, gs.MaxWave, gs.Kill, gs.Coin, gs.HP);
    DrawTextGL(startX + 12, footerStartY + 21 + fontOffsetY, gsText, 130, 130, 130, 255);

    // Watermark "BY MOLZRD" ditaruh di pojok kanan bawah footer secara elegan dengan warna merah khas P5
    DrawTextGL(menuWidth - 100, footerStartY + 21 + fontOffsetY, "BY MOLZRD", 220, 10, 40, 255);

    // === RESTORE STATE GRAPHIC SYSTEM ===
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

extern "C" void __cdecl LogSwap() { swapCount++; }
extern "C" void __cdecl LogPresent() { presentCount++; }
extern "C" void __cdecl LogLayer() { layerCount++; }

extern "C" void __cdecl RenderAndLog()
{
    if (g_inRender) return;
    g_inRender = true;
    g_frameCount++;
    GameState::Get().Update();
    MenuManager::HandleNavigation();
    if (MenuManager::IsVisible()) RenderMenuGL();
    if (g_frameCount == 1 || g_frameCount % 600 == 0) {
        char log[256];
        sprintf_s(log, sizeof(log), "[FRAME] #%d | S:%d P:%d L:%d", g_frameCount, swapCount, presentCount, layerCount);
        Logger::Log(log);
    }
    g_inRender = false;
}

extern "C" void hkSwap();
extern "C" void hkPresent();
extern "C" void hkLayer();

DWORD WINAPI HookThread(LPVOID)
{
    Logger::Log("[HK] Thread..."); Sleep(3000);
    MH_Initialize();

    // HANYA 2 HOOK!
    HMODULE hNV = GetModuleHandleA("nvoglv32.dll");
    if (hNV) {
        void* pLayer = GetProcAddress(hNV, "DrvSwapLayerBuffers");
        if (pLayer) { MH_CreateHook(pLayer, &hkLayer, &pOriginalLayer); MH_EnableHook(pLayer); Logger::Log("[HK] Layer OK"); }
    }

    HMODULE hGL = GetModuleHandleA("opengl32.dll");
    if (hGL) {
        void* pFinish = GetProcAddress(hGL, "glFinish");
        if (pFinish) { MH_CreateHook(pFinish, &hkSwap, &pOriginalSwap); MH_EnableHook(pFinish); Logger::Log("[HK] glFinish OK"); }
    }

    Logger::Log("[HK] *** 2 HOOKS! ***");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {

        // Cari base address Fieldrunners2.dll
        HMODULE hFR2 = GetModuleHandleA("Fieldrunners2.dll");
        if (hFR2) {
            g_moduleBase = (uintptr_t)hFR2;
        }

        // Initialize GameState SDK
        GameState::Get().Initialize(g_moduleBase);

        Coin::Initialize(g_moduleBase);

        MenuManager::Build();
        Logger::Initialize();
        Logger::Log("[DLL] ATTACH");

        // Log Address
        char log[256];
        sprintf_s(log, sizeof(log), "[DLL] Module base: 0x%X", g_moduleBase);
        Logger::Log(log);

        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr));
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        Logger::Log("[DLL] DETACH"); Health::Shutdown(); Sleep(100);
        MH_Uninitialize();
        if (g_fontBase) glDeleteLists(g_fontBase, 128);
        Logger::Shutdown();
    }
    return TRUE;
}