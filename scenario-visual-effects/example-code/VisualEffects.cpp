#include "D3D11Hook.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

// Static member initialization
ID3D11PixelShader* VisualEffects::pPostProcessShader = nullptr;
ID3D11VertexShader* VisualEffects::pFullscreenVS = nullptr;
ID3D11Buffer* VisualEffects::pConstantBuffer = nullptr;
ID3D11SamplerState* VisualEffects::pSamplerState = nullptr;
ID3D11BlendState* VisualEffects::pBlendState = nullptr;
ID3D11RasterizerState* VisualEffects::pRasterState = nullptr;
ID3D11DepthStencilState* VisualEffects::pDepthStencilState = nullptr;
ID3D11Texture2D* VisualEffects::pTempTexture = nullptr;
ID3D11RenderTargetView* VisualEffects::pTempRTV = nullptr;
ID3D11ShaderResourceView* VisualEffects::pTempSRV = nullptr;
EffectParams VisualEffects::currentParams = {};
bool VisualEffects::isEnabled = true;

// Post-processing shader source
const char* postProcessShaderSource = R"(
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer EffectParams : register(b0) {
    float brightness;
    float contrast;
    float saturation;
    float gamma;
    float3 colorTint;
    float padding1;
    
    float enableSepia;
    float enableGrayscale;
    float enableInvert;
    float enableVignette;
    
    float bloomStrength;
    float vignetteStrength;
    float sharpenStrength;
    float noiseStrength;
    
    float3 shadows;
    float padding2;
    float3 midtones;
    float padding3;
    float3 highlights;
    float padding4;
};

Texture2D MainTexture : register(t0);
SamplerState MainSampler : register(s0);

// Utility functions
float3 rgb2hsv(float3 c) {
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 hsv2rgb(float3 c) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float luminance(float3 color) {
    return dot(color, float3(0.299, 0.587, 0.114));
}

float3 ApplyColorGrading(float3 color) {
    // Brightness
    color *= brightness;
    
    // Contrast
    color = ((color - 0.5) * contrast) + 0.5;
    
    // Saturation
    float gray = luminance(color);
    color = lerp(gray.xxx, color, saturation);
    
    // Gamma correction
    color = pow(abs(color), gamma);
    
    // Color tint
    color *= colorTint;
    
    return color;
}

float3 ApplyColorCorrection(float3 color) {
    float lum = luminance(color);
    
    // Three-way color correction
    float3 result = color;
    
    // Shadows (darker areas)
    float shadowMask = 1.0 - smoothstep(0.0, 0.5, lum);
    result = lerp(result, result * shadows + shadows * 0.1, shadowMask);
    
    // Midtones
    float midtoneMask = sin(lum * 3.14159);
    result = lerp(result, result * midtones, midtoneMask * 0.5);
    
    // Highlights (brighter areas)
    float highlightMask = smoothstep(0.5, 1.0, lum);
    result = lerp(result, result * highlights + highlights * 0.1, highlightMask);
    
    return result;
}

float3 ApplySepia(float3 color) {
    float3 sepia;
    sepia.r = dot(color, float3(0.393, 0.769, 0.189));
    sepia.g = dot(color, float3(0.349, 0.686, 0.168));
    sepia.b = dot(color, float3(0.272, 0.534, 0.131));
    return sepia;
}

float3 ApplyVignette(float3 color, float2 uv) {
    float2 center = uv - 0.5;
    float vignette = 1.0 - dot(center, center) * vignetteStrength;
    vignette = smoothstep(0.0, 1.0, vignette);
    return color * vignette;
}

float3 ApplySharpen(float3 color, float2 uv) {
    if (sharpenStrength <= 0.0) return color;
    
    float2 texelSize = 1.0 / float2(1920, 1080); // Should be dynamic
    
    float3 blur = 0;
    blur += MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, -texelSize.y)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(0, -texelSize.y)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(texelSize.x, -texelSize.y)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, 0)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(texelSize.x, 0)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, texelSize.y)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(0, texelSize.y)).rgb;
    blur += MainTexture.Sample(MainSampler, uv + float2(texelSize.x, texelSize.y)).rgb;
    blur /= 8.0;
    
    return color + (color - blur) * sharpenStrength;
}

float random(float2 uv) {
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float3 ApplyNoise(float3 color, float2 uv) {
    if (noiseStrength <= 0.0) return color;
    
    float noise = random(uv) * 2.0 - 1.0;
    return color + noise * noiseStrength;
}

float4 main(VS_OUTPUT input) : SV_Target {
    float3 color = MainTexture.Sample(MainSampler, input.uv).rgb;
    
    // Apply sharpening first (requires original texture samples)
    color = ApplySharpen(color, input.uv);
    
    // Color grading
    color = ApplyColorGrading(color);
    
    // Color correction
    color = ApplyColorCorrection(color);
    
    // Special effects
    if (enableSepia > 0.5) {
        color = ApplySepia(color);
    }
    
    if (enableGrayscale > 0.5) {
        float gray = luminance(color);
        color = gray.xxx;
    }
    
    if (enableInvert > 0.5) {
        color = 1.0 - color;
    }
    
    // Vignette effect
    if (vignetteStrength > 0.0) {
        color = ApplyVignette(color, input.uv);
    }
    
    // Film grain/noise
    color = ApplyNoise(color, input.uv);
    
    // Clamp to valid range
    color = saturate(color);
    
    return float4(color, 1.0);
}
)";

const char* fullscreenVertexShaderSource = R"(
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main(uint id : SV_VertexID) {
    VS_OUTPUT output;
    
    // Generate full-screen triangle
    output.uv = float2((id << 1) & 2, id & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.pos.y = -output.pos.y; // Flip Y for D3D11
    
    return output;
}
)";

bool VisualEffects::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (!device || !context) {
        std::cout << "Invalid D3D11 device or context" << std::endl;
        return false;
    }
    
    std::cout << "Initializing Visual Effects System..." << std::endl;
    
    if (!CreateShaders()) {
        std::cout << "Failed to create shaders" << std::endl;
        return false;
    }
    
    if (!CreateRenderStates()) {
        std::cout << "Failed to create render states" << std::endl;
        return false;
    }
    
    if (!CreateRenderTargets()) {
        std::cout << "Failed to create render targets" << std::endl;
        return false;
    }
    
    // Create constant buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(EffectParams);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &pConstantBuffer);
    if (FAILED(hr)) {
        std::cout << "Failed to create constant buffer" << std::endl;
        return false;
    }
    
    std::cout << "Visual Effects System initialized successfully" << std::endl;
    return true;
}

bool VisualEffects::CreateShaders() {
    ID3D11Device* device = D3D11Hook::GetDevice();
    if (!device) return false;
    
    HRESULT hr;
    ID3DBlob* pShaderBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;
    
    // Compile pixel shader
    hr = D3DCompile(
        postProcessShaderSource, strlen(postProcessShaderSource),
        nullptr, nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pShaderBlob, &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            std::cout << "Pixel shader compilation error: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
            pErrorBlob->Release();
        }
        return false;
    }
    
    hr = device->CreatePixelShader(
        pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(),
        nullptr, &pPostProcessShader
    );
    pShaderBlob->Release();
    
    if (FAILED(hr)) {
        std::cout << "Failed to create pixel shader" << std::endl;
        return false;
    }
    
    // Compile vertex shader
    hr = D3DCompile(
        fullscreenVertexShaderSource, strlen(fullscreenVertexShaderSource),
        nullptr, nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pShaderBlob, &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            std::cout << "Vertex shader compilation error: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
            pErrorBlob->Release();
        }
        return false;
    }
    
    hr = device->CreateVertexShader(
        pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(),
        nullptr, &pFullscreenVS
    );
    pShaderBlob->Release();
    
    if (FAILED(hr)) {
        std::cout << "Failed to create vertex shader" << std::endl;
        return false;
    }
    
    return true;
}

bool VisualEffects::CreateRenderStates() {
    ID3D11Device* device = D3D11Hook::GetDevice();
    if (!device) return false;
    
    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    HRESULT hr = device->CreateSamplerState(&samplerDesc, &pSamplerState);
    if (FAILED(hr)) return false;
    
    // Create blend state (no blending for post-process)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = device->CreateBlendState(&blendDesc, &pBlendState);
    if (FAILED(hr)) return false;
    
    // Create rasterizer state
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = 0;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = FALSE;
    rasterDesc.ScissorEnable = FALSE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;
    
    hr = device->CreateRasterizerState(&rasterDesc, &pRasterState);
    if (FAILED(hr)) return false;
    
    // Create depth stencil state (disable depth testing)
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.StencilEnable = FALSE;
    
    hr = device->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);
    if (FAILED(hr)) return false;
    
    return true;
}

bool VisualEffects::CreateRenderTargets() {
    ID3D11Device* device = D3D11Hook::GetDevice();
    IDXGISwapChain* swapChain = D3D11Hook::GetSwapChain();
    if (!device || !swapChain) return false;
    
    // Get backbuffer description
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr)) return false;
    
    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBuffer->GetDesc(&backBufferDesc);
    pBackBuffer->Release();
    
    // Create temporary texture for multi-pass effects
    D3D11_TEXTURE2D_DESC tempTexDesc = backBufferDesc;
    tempTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    tempTexDesc.Usage = D3D11_USAGE_DEFAULT;
    tempTexDesc.CPUAccessFlags = 0;
    
    hr = device->CreateTexture2D(&tempTexDesc, nullptr, &pTempTexture);
    if (FAILED(hr)) return false;
    
    // Create render target view
    hr = device->CreateRenderTargetView(pTempTexture, nullptr, &pTempRTV);
    if (FAILED(hr)) return false;
    
    // Create shader resource view
    hr = device->CreateShaderResourceView(pTempTexture, nullptr, &pTempSRV);
    if (FAILED(hr)) return false;
    
    return true;
}

void VisualEffects::ApplyEffects() {
    if (!isEnabled || !pPostProcessShader || !pFullscreenVS) return;
    
    ID3D11DeviceContext* context = D3D11Hook::GetContext();
    if (!context) return;
    
    // Update constant buffer
    UpdateConstantBuffer();
    
    // Setup render state
    SetupRenderState();
    
    // Render fullscreen quad with post-process shader
    RenderFullscreenQuad();
    
    // Restore render state
    RestoreRenderState();
}

void VisualEffects::UpdateConstantBuffer() {
    ID3D11DeviceContext* context = D3D11Hook::GetContext();
    if (!context || !pConstantBuffer) return;
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        memcpy(mappedResource.pData, &currentParams, sizeof(EffectParams));
        context->Unmap(pConstantBuffer, 0);
    }
}

void VisualEffects::SetupRenderState() {
    ID3D11DeviceContext* context = D3D11Hook::GetContext();
    if (!context) return;
    
    // Set shaders
    context->VSSetShader(pFullscreenVS, nullptr, 0);
    context->PSSetShader(pPostProcessShader, nullptr, 0);
    
    // Set constant buffer
    context->PSSetConstantBuffers(0, 1, &pConstantBuffer);
    
    // Set sampler
    context->PSSetSamplers(0, 1, &pSamplerState);
    
    // Set render states
    context->RSSetState(pRasterState);
    context->OMSetBlendState(pBlendState, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(pDepthStencilState, 0);
    
    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr); // No input layout needed for fullscreen triangle
}

void VisualEffects::RestoreRenderState() {
    // Note: In a real implementation, you would save and restore the previous state
    // For simplicity, we're not doing that here
}

void VisualEffects::RenderFullscreenQuad() {
    ID3D11DeviceContext* context = D3D11Hook::GetContext();
    if (!context) return;
    
    // Draw fullscreen triangle (3 vertices, no vertex buffer needed)
    context->Draw(3, 0);
}

void VisualEffects::SetEffectParams(const EffectParams& params) {
    currentParams = params;
}

void VisualEffects::LoadPreset(const std::string& presetName) {
    if (presetName == "cinematic") {
        currentParams = EffectPresets::GetCinematicPreset();
    } else if (presetName == "vintage") {
        currentParams = EffectPresets::GetVintagePreset();
    } else if (presetName == "high_contrast") {
        currentParams = EffectPresets::GetHighContrastPreset();
    } else if (presetName == "warm") {
        currentParams = EffectPresets::GetWarmPreset();
    } else if (presetName == "cool") {
        currentParams = EffectPresets::GetCoolPreset();
    } else if (presetName == "dramatic") {
        currentParams = EffectPresets::GetDramaticPreset();
    } else if (presetName == "natural") {
        currentParams = EffectPresets::GetNaturalPreset();
    } else if (presetName == "bw") {
        currentParams = EffectPresets::GetBlackAndWhitePreset();
    } else if (presetName == "sepia") {
        currentParams = EffectPresets::GetSepiaPreset();
    } else if (presetName == "cyberpunk") {
        currentParams = EffectPresets::GetCyberpunkPreset();
    }
    
    std::cout << "Loaded preset: " << presetName << std::endl;
}

void VisualEffects::Shutdown() {
    if (pPostProcessShader) { pPostProcessShader->Release(); pPostProcessShader = nullptr; }
    if (pFullscreenVS) { pFullscreenVS->Release(); pFullscreenVS = nullptr; }
    if (pConstantBuffer) { pConstantBuffer->Release(); pConstantBuffer = nullptr; }
    if (pSamplerState) { pSamplerState->Release(); pSamplerState = nullptr; }
    if (pBlendState) { pBlendState->Release(); pBlendState = nullptr; }
    if (pRasterState) { pRasterState->Release(); pRasterState = nullptr; }
    if (pDepthStencilState) { pDepthStencilState->Release(); pDepthStencilState = nullptr; }
    if (pTempTexture) { pTempTexture->Release(); pTempTexture = nullptr; }
    if (pTempRTV) { pTempRTV->Release(); pTempRTV = nullptr; }
    if (pTempSRV) { pTempSRV->Release(); pTempSRV = nullptr; }
    
    std::cout << "Visual Effects System shut down" << std::endl;
}

// Effect Presets Implementation
namespace EffectPresets {
    EffectParams GetCinematicPreset() {
        EffectParams params = {};
        params.brightness = 0.9f;
        params.contrast = 1.2f;
        params.saturation = 1.1f;
        params.gamma = 1.1f;
        params.colorTint = {1.05f, 1.0f, 0.95f}; // Slightly warm
        params.vignetteStrength = 0.3f;
        params.shadows = {0.95f, 0.98f, 1.0f}; // Lift shadows with blue tint
        params.highlights = {1.0f, 1.0f, 0.98f}; // Warm highlights
        return params;
    }
    
    EffectParams GetVintagePreset() {
        EffectParams params = {};
        params.brightness = 0.95f;
        params.contrast = 1.15f;
        params.saturation = 0.8f;
        params.gamma = 1.05f;
        params.colorTint = {1.1f, 1.0f, 0.9f}; // Warm tone
        params.enableSepia = 0.5f; // Partial sepia
        params.vignetteStrength = 0.4f;
        params.noiseStrength = 0.02f; // Film grain
        return params;
    }
    
    EffectParams GetHighContrastPreset() {
        EffectParams params = {};
        params.brightness = 1.1f;
        params.contrast = 1.8f;
        params.saturation = 1.3f;
        params.gamma = 0.9f;
        params.colorTint = {1.0f, 1.0f, 1.0f};
        params.sharpenStrength = 0.5f;
        params.shadows = {0.9f, 0.9f, 0.9f}; // Lift shadows
        params.highlights = {1.1f, 1.1f, 1.1f}; // Boost highlights
        return params;
    }
    
    EffectParams GetWarmPreset() {
        EffectParams params = {};
        params.brightness = 1.0f;
        params.contrast = 1.1f;
        params.saturation = 1.15f;
        params.gamma = 1.0f;
        params.colorTint = {1.15f, 1.05f, 0.9f}; // Warm orange tint
        params.shadows = {1.0f, 0.98f, 0.95f}; // Warm shadows
        params.midtones = {1.05f, 1.0f, 0.98f}; // Warm midtones
        return params;
    }
    
    EffectParams GetCoolPreset() {
        EffectParams params = {};
        params.brightness = 1.0f;
        params.contrast = 1.1f;
        params.saturation = 1.1f;
        params.gamma = 1.0f;
        params.colorTint = {0.9f, 1.0f, 1.15f}; // Cool blue tint
        params.shadows = {0.95f, 0.98f, 1.05f}; // Cool shadows
        params.highlights = {0.98f, 1.0f, 1.02f}; // Cool highlights
        return params;
    }
    
    EffectParams GetDramaticPreset() {
        EffectParams params = {};
        params.brightness = 0.85f;
        params.contrast = 1.6f;
        params.saturation = 1.4f;
        params.gamma = 0.85f;
        params.colorTint = {1.0f, 0.98f, 0.95f};
        params.vignetteStrength = 0.5f;
        params.shadows = {0.8f, 0.85f, 0.9f}; // Crush shadows
        params.highlights = {1.2f, 1.15f, 1.1f}; // Boost highlights
        return params;
    }
    
    EffectParams GetNaturalPreset() {
        EffectParams params = {};
        params.brightness = 1.0f;
        params.contrast = 1.05f;
        params.saturation = 1.0f;
        params.gamma = 1.0f;
        params.colorTint = {1.0f, 1.0f, 1.0f};
        params.sharpenStrength = 0.1f; // Subtle sharpening
        return params;
    }
    
    EffectParams GetBlackAndWhitePreset() {
        EffectParams params = {};
        params.brightness = 1.0f;
        params.contrast = 1.3f;
        params.saturation = 0.0f; // Will be overridden by grayscale
        params.gamma = 1.0f;
        params.enableGrayscale = 1.0f;
        params.sharpenStrength = 0.2f;
        return params;
    }
    
    EffectParams GetSepiaPreset() {
        EffectParams params = {};
        params.brightness = 1.0f;
        params.contrast = 1.1f;
        params.saturation = 0.8f;
        params.gamma = 1.05f;
        params.enableSepia = 1.0f;
        params.vignetteStrength = 0.3f;
        return params;
    }
    
    EffectParams GetCyberpunkPreset() {
        EffectParams params = {};
        params.brightness = 1.1f;
        params.contrast = 1.4f;
        params.saturation = 1.5f;
        params.gamma = 0.9f;
        params.colorTint = {1.0f, 0.95f, 1.1f}; // Magenta tint
        params.sharpenStrength = 0.4f;
        params.shadows = {0.9f, 0.8f, 1.0f}; // Blue shadows
        params.highlights = {1.1f, 1.0f, 1.05f}; // Magenta highlights
        return params;
    }
}