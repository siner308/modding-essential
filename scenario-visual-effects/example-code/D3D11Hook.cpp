#include "D3D11Hook.h"
#include <iostream>
#include <thread>
#include <chrono>

// Static member initialization
ID3D11Device* D3D11Hook::pDevice = nullptr;
ID3D11DeviceContext* D3D11Hook::pContext = nullptr;
IDXGISwapChain* D3D11Hook::pSwapChain = nullptr;
ID3D11RenderTargetView* D3D11Hook::pMainRTV = nullptr;
ID3D11Texture2D* D3D11Hook::pBackBuffer = nullptr;

D3D11Hook::Present_t D3D11Hook::oPresent = nullptr;
D3D11Hook::PSSetShader_t D3D11Hook::oPSSetShader = nullptr;
D3D11Hook::Draw_t D3D11Hook::oDraw = nullptr;
D3D11Hook::DrawIndexed_t D3D11Hook::oDrawIndexed = nullptr;

bool D3D11Hook::isInitialized = false;

// External library functions (normally from detours or similar)
extern "C" {
    // Simplified hook implementation - in real usage, use Microsoft Detours or similar
    bool InstallHook(void* target, void* detour, void** original);
    bool RemoveHook(void* target, void* original);
}

bool D3D11Hook::Initialize() {
    if (isInitialized) {
        std::cout << "D3D11Hook already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Initializing DirectX 11 Hook..." << std::endl;
    
    // Create temporary D3D11 device to get VTable addresses
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_DEBUG,
        nullptr, 0, D3D11_SDK_VERSION,
        &pDevice, &featureLevel, &pContext
    );
    
    if (FAILED(hr)) {
        std::cout << "Failed to create D3D11 device for hooking" << std::endl;
        return false;
    }
    
    // Create swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 1;
    swapChainDesc.BufferDesc.Height = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetDesktopWindow();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    
    IDXGIDevice* dxgiDevice = nullptr;
    hr = pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        std::cout << "Failed to get DXGI device" << std::endl;
        return false;
    }
    
    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get DXGI adapter" << std::endl;
        return false;
    }
    
    IDXGIFactory* dxgiFactory = nullptr;
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    dxgiAdapter->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to get DXGI factory" << std::endl;
        return false;
    }
    
    hr = dxgiFactory->CreateSwapChain(pDevice, &swapChainDesc, &pSwapChain);
    dxgiFactory->Release();
    if (FAILED(hr)) {
        std::cout << "Failed to create swap chain" << std::endl;
        return false;
    }
    
    // Hook DirectX functions
    if (!HookDirectX()) {
        std::cout << "Failed to hook DirectX functions" << std::endl;
        return false;
    }
    
    // Initialize Visual Effects
    if (!VisualEffects::Initialize(pDevice, pContext)) {
        std::cout << "Failed to initialize Visual Effects" << std::endl;
        return false;
    }
    
    isInitialized = true;
    std::cout << "DirectX 11 Hook initialized successfully" << std::endl;
    return true;
}

void D3D11Hook::Shutdown() {
    if (!isInitialized) return;
    
    std::cout << "Shutting down DirectX 11 Hook..." << std::endl;
    
    // Shutdown Visual Effects
    VisualEffects::Shutdown();
    
    // Remove hooks
    if (oPresent) {
        RemoveHook(GetVTableFunction(pSwapChain, 8), (void*)oPresent);
    }
    if (oPSSetShader) {
        RemoveHook(GetVTableFunction(pContext, 9), (void*)oPSSetShader);
    }
    if (oDraw) {
        RemoveHook(GetVTableFunction(pContext, 13), (void*)oDraw);
    }
    if (oDrawIndexed) {
        RemoveHook(GetVTableFunction(pContext, 12), (void*)oDrawIndexed);
    }
    
    // Release DirectX objects
    if (pMainRTV) { pMainRTV->Release(); pMainRTV = nullptr; }
    if (pBackBuffer) { pBackBuffer->Release(); pBackBuffer = nullptr; }
    if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
    if (pContext) { pContext->Release(); pContext = nullptr; }
    if (pDevice) { pDevice->Release(); pDevice = nullptr; }
    
    isInitialized = false;
    std::cout << "DirectX 11 Hook shut down" << std::endl;
}

bool D3D11Hook::HookDirectX() {
    // Get VTable function addresses
    void* presentAddr = GetVTableFunction(pSwapChain, 8);  // Present
    void* psSetShaderAddr = GetVTableFunction(pContext, 9); // PSSetShader
    void* drawAddr = GetVTableFunction(pContext, 13);      // Draw
    void* drawIndexedAddr = GetVTableFunction(pContext, 12); // DrawIndexed
    
    if (!presentAddr || !psSetShaderAddr) {
        std::cout << "Failed to get VTable addresses" << std::endl;
        return false;
    }
    
    // Install hooks
    if (!InstallHook(presentAddr, (void*)hkPresent, (void**)&oPresent)) {
        std::cout << "Failed to hook Present" << std::endl;
        return false;
    }
    
    if (!InstallHook(psSetShaderAddr, (void*)hkPSSetShader, (void**)&oPSSetShader)) {
        std::cout << "Failed to hook PSSetShader" << std::endl;
        return false;
    }
    
    if (!InstallHook(drawAddr, (void*)hkDraw, (void**)&oDraw)) {
        std::cout << "Failed to hook Draw" << std::endl;
        return false;
    }
    
    if (!InstallHook(drawIndexedAddr, (void*)hkDrawIndexed, (void**)&oDrawIndexed)) {
        std::cout << "Failed to hook DrawIndexed" << std::endl;
        return false;
    }
    
    return true;
}

void* D3D11Hook::GetVTableFunction(void* instance, int index) {
    if (!instance) return nullptr;
    
    void** vtable = *(void***)instance;
    return vtable[index];
}

HRESULT __stdcall D3D11Hook::hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    // This is called every frame - perfect place to inject our effects
    static bool firstCall = true;
    
    if (firstCall) {
        std::cout << "First Present() call - effects will be applied" << std::endl;
        
        // Get back buffer for effects
        HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (SUCCEEDED(hr)) {
            pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRTV);
        }
        
        firstCall = false;
    }
    
    // Apply visual effects before presenting
    if (VisualEffects::IsEnabled()) {
        VisualEffects::ApplyEffects();
    }
    
    return oPresent(pSwapChain, SyncInterval, Flags);
}

void __stdcall D3D11Hook::hkPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader,
                                       ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
    // This can be used for selective shader replacement
    // For now, just call original
    oPSSetShader(pContext, pPixelShader, ppClassInstances, NumClassInstances);
}

void __stdcall D3D11Hook::hkDraw(ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation) {
    // Hook draw calls for specific effect injection
    oDraw(pContext, VertexCount, StartVertexLocation);
}

void __stdcall D3D11Hook::hkDrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, 
                                       UINT StartIndexLocation, INT BaseVertexLocation) {
    // Hook indexed draw calls
    oDrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

// ShaderManager Implementation
std::vector<ShaderManager::ShaderInfo> ShaderManager::loadedShaders;
bool ShaderManager::hotReloadEnabled = false;

bool ShaderManager::Initialize() {
    std::cout << "Initializing Shader Manager..." << std::endl;
    loadedShaders.clear();
    hotReloadEnabled = false;
    return true;
}

void ShaderManager::Shutdown() {
    std::cout << "Shutting down Shader Manager..." << std::endl;
    
    for (auto& shader : loadedShaders) {
        if (shader.shader) {
            shader.shader->Release();
        }
    }
    
    loadedShaders.clear();
}

ID3D11PixelShader* ShaderManager::CompilePixelShader(const std::string& source, const std::string& entryPoint) {
    ID3DBlob* shaderBlob = nullptr;
    
    if (!CompileShaderFromSource(source, entryPoint, "ps_5_0", &shaderBlob)) {
        return nullptr;
    }
    
    ID3D11PixelShader* shader = nullptr;
    HRESULT hr = D3D11Hook::GetDevice()->CreatePixelShader(
        shaderBlob->GetBufferPointer(), 
        shaderBlob->GetBufferSize(), 
        nullptr, 
        &shader
    );
    
    shaderBlob->Release();
    
    if (FAILED(hr)) {
        std::cout << "Failed to create pixel shader" << std::endl;
        return nullptr;
    }
    
    return shader;
}

ID3D11VertexShader* ShaderManager::CompileVertexShader(const std::string& source, const std::string& entryPoint) {
    ID3DBlob* shaderBlob = nullptr;
    
    if (!CompileShaderFromSource(source, entryPoint, "vs_5_0", &shaderBlob)) {
        return nullptr;
    }
    
    ID3D11VertexShader* shader = nullptr;
    HRESULT hr = D3D11Hook::GetDevice()->CreateVertexShader(
        shaderBlob->GetBufferPointer(), 
        shaderBlob->GetBufferSize(), 
        nullptr, 
        &shader
    );
    
    shaderBlob->Release();
    
    if (FAILED(hr)) {
        std::cout << "Failed to create vertex shader" << std::endl;
        return nullptr;
    }
    
    return shader;
}

bool ShaderManager::CompileShaderFromSource(const std::string& source, const std::string& entryPoint,
                                          const std::string& profile, ID3DBlob** outBlob) {
    ID3DBlob* errorBlob = nullptr;
    
    HRESULT hr = D3DCompile(
        source.c_str(), source.length(),
        nullptr, nullptr, nullptr,
        entryPoint.c_str(), profile.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        outBlob, &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            std::cout << "Shader compilation error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return false;
    }
    
    return true;
}

// EffectProfiler Implementation
std::vector<float> EffectProfiler::frameTimes;
std::chrono::high_resolution_clock::time_point EffectProfiler::lastFrame;
float EffectProfiler::averageFrameTime = 0.0f;
int EffectProfiler::frameCount = 0;

void EffectProfiler::BeginFrame() {
    lastFrame = std::chrono::high_resolution_clock::now();
}

void EffectProfiler::EndFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float, std::milli>(now - lastFrame).count();
    
    frameTimes.push_back(frameTime);
    if (frameTimes.size() > 120) { // Keep last 2 seconds at 60 FPS
        frameTimes.erase(frameTimes.begin());
    }
    
    // Calculate average
    float total = 0.0f;
    for (float time : frameTimes) {
        total += time;
    }
    averageFrameTime = total / frameTimes.size();
    
    frameCount++;
}

void EffectProfiler::Reset() {
    frameTimes.clear();
    averageFrameTime = 0.0f;
    frameCount = 0;
}

// Simplified hook implementation stubs
// In real implementation, use Microsoft Detours or similar hooking library
extern "C" {
    bool InstallHook(void* target, void* detour, void** original) {
        // This would use a proper hooking library like Microsoft Detours
        // For educational purposes, we're showing the interface
        std::cout << "Hook installed (stub implementation)" << std::endl;
        *original = target; // In real implementation, this would be the original function
        return true;
    }
    
    bool RemoveHook(void* target, void* original) {
        // Remove the hook and restore original function
        std::cout << "Hook removed (stub implementation)" << std::endl;
        return true;
    }
}