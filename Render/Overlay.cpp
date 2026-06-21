#include "Overlay.hpp"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>

namespace ESP {
    Overlay::Overlay() = default;

    Overlay::~Overlay() {
        Cleanup();
    }

    bool Overlay::Initialize() {
        if (!CreateTransparentWindow()) {
            std::cerr << "[Overlay] Failed to create window." << std::endl;
            return false;
        }

        if (!InitializeDirect3D()) {
            std::cerr << "[Overlay] Failed to initialize Direct3D." << std::endl;
            return false;
        }

        InitializeImGui();
        return true;
    }

    bool Overlay::CreateTransparentWindow() {
        m_hWndTarget = FindWindowW(nullptr, L"Counter-Strike 2");
        if (!m_hWndTarget) {
            std::cerr << "[Overlay] Target window not found." << std::endl;
            return false;
        }

        // Register window class
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpszClassName = L"CS2_ESP_OVERLAY";
        wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
            return DefWindowProcW(hwnd, msg, wp, lp);
            };
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if (!RegisterClassExW(&wc)) {
            std::cerr << "[Overlay] Failed to register window class." << std::endl;
            return false;
        }

        // Create window
        RECT targetRect;
        GetWindowRect(m_hWndTarget, &targetRect);

        m_hWndOverlay = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            wc.lpszClassName,
            L"",
            WS_POPUP,
            targetRect.left,
            targetRect.top,
            targetRect.right - targetRect.left,
            targetRect.bottom - targetRect.top,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        if (!m_hWndOverlay) {
            std::cerr << "[Overlay] Failed to create window." << std::endl;
            return false;
        }

        // Set transparency
        SetLayeredWindowAttributes(m_hWndOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(m_hWndOverlay, SW_SHOW);

        return true;
    }

    bool Overlay::InitializeDirect3D() {
        RECT rect;
        GetClientRect(m_hWndTarget, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hWndOverlay;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &sd,
            &m_pSwapChain,
            &m_pDevice,
            &featureLevel,
            &m_pContext
        );

        if (FAILED(hr)) {
            std::cerr << "[Overlay] D3D11CreateDeviceAndSwapChain failed. Error: " << std::hex << hr << std::endl;
            return false;
        }

        // Create render target view
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr)) {
            std::cerr << "[Overlay] Failed to get back buffer." << std::endl;
            return false;
        }

        hr = m_pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_pRenderTargetView);
        if (FAILED(hr)) {
            std::cerr << "[Overlay] Failed to create render target view." << std::endl;
            return false;
        }

        return true;
    }

    void Overlay::InitializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(m_hWndOverlay);
        ImGui_ImplDX11_Init(m_pDevice.Get(), m_pContext.Get());
        ImGui::GetIO().IniFilename = nullptr;  // Disable imgui.ini
    }

    void Overlay::UpdateWindowPosition() {
        RECT targetRect;
        GetWindowRect(m_hWndTarget, &targetRect);

        RECT currentRect;
        GetWindowRect(m_hWndOverlay, &currentRect);

        if (memcmp(&currentRect, &targetRect, sizeof(RECT)) != 0) {
            SetWindowPos(
                m_hWndOverlay,
                HWND_TOPMOST,
                targetRect.left,
                targetRect.top,
                targetRect.right - targetRect.left,
                targetRect.bottom - targetRect.top,
                SWP_NOACTIVATE
            );
        }
    }

    void Overlay::Render(DriverClient& driver) {
        UpdateWindowPosition();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ESPData data;
        driver.GetData(data);

        // Récupère les dimensions de la fenêtre cible
        RECT rect;
        GetClientRect(m_hWndTarget, &rect);
        int screenWidth = rect.right - rect.left;
        int screenHeight = rect.bottom - rect.top;

        // Appel corrigé avec screenWidth et screenHeight
        m_rendering.DrawESP(data, ImGui::GetBackgroundDrawList(), screenWidth, screenHeight);

        ImGui::EndFrame();
        ImGui::Render();

        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_pContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);
        m_pContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_pSwapChain->Present(1, 0);
    }

    void Overlay::Cleanup() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (m_hWndOverlay) {
            DestroyWindow(m_hWndOverlay);
            m_hWndOverlay = nullptr;
        }
    }

    HWND Overlay::GetWindowHandle() const {
        return m_hWndOverlay;
    }
}