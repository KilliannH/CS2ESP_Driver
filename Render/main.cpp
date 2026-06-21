#include "DriverClient.hpp"
#include "Overlay.hpp"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 1. Initialisation du client driver
    ESP::DriverClient driver;
    if (!driver.Initialize()) {
        MessageBoxA(nullptr, "Failed to open driver. Make sure it's loaded.", "Error", MB_ICONERROR);
        return 1;
    }

    // 2. Initialisation de l'overlay
    ESP::Overlay overlay;
    if (!overlay.Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize overlay (DX11/WinAPI).", "Error", MB_ICONERROR);
        return 1;
    }

    // 3. Boucle principale
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            overlay.Render(driver);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    overlay.Cleanup();
    return 0;
}