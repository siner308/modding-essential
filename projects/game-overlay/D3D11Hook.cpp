#include "D3D11Hook.h"
#include "vendor/minhook/include/MinHook.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_dx11.h"
#include "vendor/imgui/imgui_impl_win32.h"
#include <d3d11.h>
#include <iostream>

// Original Present function pointer
typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t oPresent = nullptr;

// Hooked Present function
HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool init = false;
    if (!init) {
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
        pDevice->GetImmediateContext(&pContext);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(/* Get HWND */);
        ImGui_ImplDX11_Init(pDevice, pContext);
        init = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Render your UI
    ImGui::Begin("My Overlay");
    ImGui::Text("Hello, world!");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void D3D11Hook::Initialize() {
    // Find the Present function address
    // This is a simplified example. A real implementation would need to find the correct address.
    void* pPresent = nullptr; // Placeholder

    if (MH_Initialize() != MH_OK) {
        std::cerr << "MinHook initialization failed" << std::endl;
        return;
    }

    if (MH_CreateHook(pPresent, &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) {
        std::cerr << "Failed to create hook for Present" << std::endl;
        return;
    }

    if (MH_EnableHook(pPresent) != MH_OK) {
        std::cerr << "Failed to enable hook for Present" << std::endl;
        return;
    }

    std::cout << "D3D11 Hook Initialized" << std::endl;
}

void D3D11Hook::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    std::cout << "D3D11 Hook Shutdown" << std::endl;
}
