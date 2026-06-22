#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

#include <windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

#include "DriverClient.hpp"
#include "ModuleHelper.hpp"

// ---- Global state ----
static Demo::DriverClient g_client;
static DWORD              g_pid        = 0;
static std::uint64_t      g_clientBase = 0;
static Cs2EspData         g_espData{};
static CRITICAL_SECTION   g_cs;
static std::atomic<bool>  g_running{ true };

// ---- World-to-Screen ----
// Returns true if the world point projects in front of the camera.
static bool WorldToScreen(const float* vm, float wx, float wy, float wz,
                          int screenW, int screenH,
                          float& outX, float& outY)
{
    // Column-major view-projection matrix from CS2
    float clipX = vm[0]*wx + vm[1]*wy + vm[2]*wz  + vm[3];
    float clipY = vm[4]*wx + vm[5]*wy + vm[6]*wz  + vm[7];
    float clipW = vm[12]*wx + vm[13]*wy + vm[14]*wz + vm[15];

    if (clipW < 0.001f) return false;

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    outX = (screenW / 2.0f) * (1.0f + ndcX);
    outY = (screenH / 2.0f) * (1.0f - ndcY);
    return true;
}

// ---- Helper: find CS2 PID ----
static DWORD FindCs2Pid()
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"cs2.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}

// ---- Data reader thread ----
static void ReaderThread()
{
    while (g_running) {
        Cs2EspData tmp{};
        if (g_client.GetCS2Data(g_pid, g_clientBase, tmp)) {
            EnterCriticalSection(&g_cs);
            g_espData = tmp;
            LeaveCriticalSection(&g_cs);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 Hz
    }
}

// ---- Overlay window procedure ----
static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Get window size
        RECT rc;
        GetClientRect(hwnd, &rc);
        int W = rc.right;
        int H = rc.bottom;

        // Double-buffered via memory DC to avoid flicker
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, W, H);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // Clear to transparent black
        HBRUSH clearBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(memDC, &rc, clearBrush);
        DeleteObject(clearBrush);

        // Snapshot ESP data
        Cs2EspData snap{};
        EnterCriticalSection(&g_cs);
        snap = g_espData;
        LeaveCriticalSection(&g_cs);

        // Pens & fonts
        HPEN boxPen   = CreatePen(PS_SOLID, 2, RGB(255, 50, 50));    // red box
        HPEN hpPen    = CreatePen(PS_SOLID, 2, RGB(50, 255, 50));     // green health
        HPEN outlinePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));       // black outline
        HFONT font = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH, L"Arial");

        SetBkMode(memDC, TRANSPARENT);
        SelectObject(memDC, font);

        for (DWORD i = 0; i < snap.entityCount && i < CS2_MAX_ENTITIES; i++) {
            const PlayerInfo& e = snap.entities[i];
            if (!e.valid || e.health <= 0) continue;

            // Project feet (entity origin)
            float fX, fY;
            if (!WorldToScreen(snap.viewMatrix, e.x, e.y, e.z, W, H, fX, fY))
                continue;

            // Project head (approximate: +64 units up in Z, matches CS2 player height)
            float hX, hY;
            if (!WorldToScreen(snap.viewMatrix, e.x, e.y, e.z + 64.0f, W, H, hX, hY))
                continue;

            // Box dimensions
            // Small padding: 2px on top and bottom so the box tightly wraps the player
            constexpr float kPad = 2.0f;
            float boxH = fY - hY;
            float boxW = boxH * 0.45f;
            int bLeft  = (int)(fX - boxW / 2);
            int bTop   = (int)(hY - kPad);
            int bRight = (int)(fX + boxW / 2);
            int bBot   = (int)(fY + kPad);

            if (boxH < 4.0f || boxH > (float)H) continue;

            // Draw black outline (slightly larger)
            SelectObject(memDC, outlinePen);
            SelectObject(memDC, GetStockObject(NULL_BRUSH));
            Rectangle(memDC, bLeft - 1, bTop - 1, bRight + 1, bBot + 1);

            // Draw red ESP box
            SelectObject(memDC, boxPen);
            Rectangle(memDC, bLeft, bTop, bRight, bBot);

            // Health bar (left of box)
            int barH = (int)(boxH * e.health / 100.0f);
            int barX = bLeft - 6;
            HBRUSH hpBrush = CreateSolidBrush(RGB(50, 255, 50));
            RECT barRect = { barX - 2, bBot - barH, barX, bBot };
            FillRect(memDC, &barRect, hpBrush);
            DeleteObject(hpBrush);

            // Health text above box
            std::wstring hpStr = std::to_wstring(e.health);
            SetTextColor(memDC, RGB(255, 255, 255));
            TextOutW(memDC, bLeft, bTop - 16, hpStr.c_str(), (int)hpStr.size());
        }

        DeleteObject(boxPen);
        DeleteObject(hpPen);
        DeleteObject(outlinePen);
        DeleteObject(font);

        // Blit to screen
        BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---- Entry point ----
int main()
{
    InitializeCriticalSection(&g_cs);

    // 1. Open driver
    if (!g_client.Initialize()) {
        MessageBoxW(NULL, L"Failed to open driver handle.\nMake sure CS2ESP_Driver is loaded and sc start CS2ESP was run.",
                    L"CS2ESP", MB_ICONERROR);
        return 1;
    }

    // 2. Find cs2.exe PID
    g_pid = FindCs2Pid();
    if (!g_pid) {
        MessageBoxW(NULL, L"cs2.exe not found. Launch the game first.", L"CS2ESP", MB_ICONERROR);
        return 1;
    }

    // 3. Find client.dll base
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < 10 && g_clientBase == 0; i++) {
        g_clientBase = Demo::GetModuleBase(g_pid, L"client.dll");
        if (!g_clientBase)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (!g_clientBase) {
        MessageBoxW(NULL, L"client.dll not found in cs2.exe.\nMake sure you are in a game (not the main menu).", L"CS2ESP", MB_ICONERROR);
        return 1;
    }

    // 4. Find CS2 window to match overlay position/size
    HWND cs2Wnd = FindWindowW(nullptr, L"Counter-Strike 2");
    RECT cs2Rect{ 0, 0, 1920, 1080 };
    if (cs2Wnd) GetWindowRect(cs2Wnd, &cs2Rect);
    int W = cs2Rect.right  - cs2Rect.left;
    int H = cs2Rect.bottom - cs2Rect.top;

    // 5. Register overlay window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = OverlayWndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"CS2ESP_Overlay";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    // 6. Create transparent, click-through overlay window
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"CS2ESP_Overlay", L"CS2ESP",
        WS_POPUP | WS_VISIBLE,
        cs2Rect.left, cs2Rect.top, W, H,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    // Black = transparent color key
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // 7. Start reader thread
    std::thread reader(ReaderThread);

    // 8. Message + render loop
    MSG msg{};
    while (g_running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 fps
    }

    g_running = false;
    reader.join();
    DeleteCriticalSection(&g_cs);
    return 0;
}