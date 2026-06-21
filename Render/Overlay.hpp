#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include "DriverClient.hpp"
#include "Rendering.hpp"

namespace ESP {
    class Overlay {
    public:
        Overlay();
        ~Overlay();

        bool Initialize();
        void Render(DriverClient& driver);
        void Cleanup();
        HWND GetWindowHandle() const;

    private:
        bool CreateTransparentWindow();
        bool InitializeDirect3D();
        void InitializeImGui();
        void UpdateWindowPosition();

        HWND m_hWndOverlay = nullptr;
        HWND m_hWndTarget = nullptr;

        Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;

        Rendering m_rendering;
    };
}