#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <vector>
#include <memory>
#include <string>

using namespace DirectX;

/**
 * DirectX 11 Hook System for Visual Effects
 * 
 * This system hooks into DirectX 11 rendering pipeline to inject
 * custom visual effects like post-processing, color grading, and filters.
 * 
 * Features:
 * - Present() hook for frame injection
 * - Shader replacement system  
 * - Real-time effect parameter adjustment
 * - Multiple effect stacking
 */

struct EffectParams {
    float brightness = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float gamma = 1.0f;
    XMFLOAT3 colorTint = {1.0f, 1.0f, 1.0f};
    float padding1 = 0.0f;
    
    // Effect flags (packed as floats for shader compatibility)
    float enableSepia = 0.0f;
    float enableGrayscale = 0.0f;
    float enableInvert = 0.0f;
    float enableVignette = 0.0f;
    
    // Advanced effects
    float bloomStrength = 0.0f;
    float vignetteStrength = 0.0f;
    float sharpenStrength = 0.0f;
    float noiseStrength = 0.0f;
    
    // Color correction
    XMFLOAT3 shadows = {0.0f, 0.0f, 0.0f};
    float padding2 = 0.0f;
    XMFLOAT3 midtones = {1.0f, 1.0f, 1.0f};
    float padding3 = 0.0f;
    XMFLOAT3 highlights = {1.0f, 1.0f, 1.0f};
    float padding4 = 0.0f;
};

class D3D11Hook {
private:
    static ID3D11Device* pDevice;
    static ID3D11DeviceContext* pContext;
    static IDXGISwapChain* pSwapChain;
    static ID3D11RenderTargetView* pMainRTV;
    static ID3D11Texture2D* pBackBuffer;
    
    // Hook function pointers
    typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
    typedef void(__stdcall* PSSetShader_t)(ID3D11DeviceContext*, ID3D11PixelShader*, 
                                          ID3D11ClassInstance* const*, UINT);
    typedef void(__stdcall* Draw_t)(ID3D11DeviceContext*, UINT, UINT);
    typedef void(__stdcall* DrawIndexed_t)(ID3D11DeviceContext*, UINT, UINT, INT);
    
    static Present_t oPresent;
    static PSSetShader_t oPSSetShader;
    static Draw_t oDraw;
    static DrawIndexed_t oDrawIndexed;
    
    static bool isInitialized;

public:
    static bool Initialize();
    static void Shutdown();
    
    // Hook functions
    static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static void __stdcall hkPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader,
                                       ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances);
    static void __stdcall hkDraw(ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation);
    static void __stdcall hkDrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, 
                                       UINT StartIndexLocation, INT BaseVertexLocation);
    
    // Getters
    static ID3D11Device* GetDevice() { return pDevice; }
    static ID3D11DeviceContext* GetContext() { return pContext; }
    static IDXGISwapChain* GetSwapChain() { return pSwapChain; }
    
private:
    static bool HookDirectX();
    static void* GetVTableFunction(void* instance, int index);
};

/**
 * Visual Effects System
 * 
 * Manages post-processing effects, shader compilation, and parameter updates.
 * Integrates with D3D11Hook to apply effects during frame rendering.
 */
class VisualEffects {
private:
    static ID3D11PixelShader* pPostProcessShader;
    static ID3D11VertexShader* pFullscreenVS;
    static ID3D11Buffer* pConstantBuffer;
    static ID3D11SamplerState* pSamplerState;
    static ID3D11BlendState* pBlendState;
    static ID3D11RasterizerState* pRasterState;
    static ID3D11DepthStencilState* pDepthStencilState;
    
    // Render targets for multi-pass effects
    static ID3D11Texture2D* pTempTexture;
    static ID3D11RenderTargetView* pTempRTV;
    static ID3D11ShaderResourceView* pTempSRV;
    
    static EffectParams currentParams;
    static bool isEnabled;

public:
    static bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    static void Shutdown();
    
    // Effect management
    static void ApplyEffects();
    static void SetEffectParams(const EffectParams& params);
    static EffectParams& GetEffectParams() { return currentParams; }
    static void SetEnabled(bool enabled) { isEnabled = enabled; }
    static bool IsEnabled() { return isEnabled; }
    
    // Shader management
    static bool RecompileShaders();
    static bool LoadShadersFromFile(const std::string& shaderPath);
    
    // Preset management
    static void LoadPreset(const std::string& presetName);
    static void SavePreset(const std::string& presetName);
    static std::vector<std::string> GetAvailablePresets();

private:
    static bool CreateShaders();
    static bool CreateRenderStates();
    static bool CreateRenderTargets();
    static void UpdateConstantBuffer();
    static void SetupRenderState();
    static void RestoreRenderState();
    
    // Multi-pass rendering
    static void RenderFullscreenQuad();
    static void ApplyBloomEffect();
    static void ApplyToneMapping();
};

/**
 * Shader Manager for runtime compilation and hot-reloading
 */
class ShaderManager {
private:
    struct ShaderInfo {
        std::string filePath;
        std::string entryPoint;
        std::string profile;
        ID3D11PixelShader* shader;
        FILETIME lastModified;
    };
    
    static std::vector<ShaderInfo> loadedShaders;
    static bool hotReloadEnabled;

public:
    static bool Initialize();
    static void Shutdown();
    
    // Shader compilation
    static ID3D11PixelShader* CompilePixelShader(const std::string& source, 
                                               const std::string& entryPoint = "main");
    static ID3D11VertexShader* CompileVertexShader(const std::string& source,
                                                 const std::string& entryPoint = "main");
    
    // File management
    static bool LoadShaderFromFile(const std::string& filePath, const std::string& entryPoint,
                                 const std::string& profile);
    static void CheckForUpdates(); // Call periodically for hot reload
    static void EnableHotReload(bool enable) { hotReloadEnabled = enable; }
    
    // Shader templates
    static std::string GetPostProcessTemplate();
    static std::string GetColorGradingTemplate();
    static std::string GetBloomTemplate();

private:
    static bool CompileShaderFromSource(const std::string& source, const std::string& entryPoint,
                                      const std::string& profile, ID3DBlob** outBlob);
    static FILETIME GetFileModifiedTime(const std::string& filePath);
};

/**
 * Effect Presets for easy switching between different visual styles
 */
namespace EffectPresets {
    EffectParams GetCinematicPreset();
    EffectParams GetVintagePreset();
    EffectParams GetHighContrastPreset();
    EffectParams GetWarmPreset();
    EffectParams GetCoolPreset();
    EffectParams GetDramaticPreset();
    EffectParams GetNaturalPreset();
    EffectParams GetBlackAndWhitePreset();
    EffectParams GetSepiaPreset();
    EffectParams GetCyberpunkPreset();
}

/**
 * Performance monitor for effect system
 */
class EffectProfiler {
private:
    static std::vector<float> frameTimes;
    static std::chrono::high_resolution_clock::time_point lastFrame;
    static float averageFrameTime;
    static int frameCount;

public:
    static void BeginFrame();
    static void EndFrame();
    static float GetAverageFrameTime() { return averageFrameTime; }
    static float GetCurrentFPS() { return 1000.0f / averageFrameTime; }
    static void Reset();
};