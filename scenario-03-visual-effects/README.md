# 🟡 Scenario 03: 시각 효과 수정 모딩

**난이도**: 중급 | **학습 시간**: 2-3주 | **접근법**: 그래픽스 파이프라인 조작

게임의 시각적 효과를 실시간으로 수정하는 고급 모딩 기법을 학습합니다.

## 📖 학습 목표

이 시나리오를 완료하면 다음을 할 수 있게 됩니다:

- [ ] DirectX/OpenGL 렌더링 파이프라인 이해하기
- [ ] 셰이더 코드 분석 및 수정하기
- [ ] 텍스처와 머티리얼 실시간 교체하기
- [ ] 포스트 프로세싱 효과 추가/제거하기
- [ ] ReShade와 같은 인젝션 기법 구현하기

## 🎯 최종 결과물

완성된 모드의 기능:
- **실시간 색상 필터** (세피아, 흑백, 비비드 등)
- **블룸/글로우 효과** 조정
- **안개/파티클 효과** 제거/추가
- **HDR/톤매핑** 파라미터 수정
- **커스텀 셰이더** 인젝션

## 🎨 그래픽스 파이프라인 이해

### 1. 렌더링 파이프라인 구조
```
게임 렌더링 파이프라인:
Input Assembly → Vertex Shader → Rasterization → Pixel Shader → Output Merger
     ↑               ↑              ↑              ↑            ↑
  기하학 데이터     정점 변환      픽셀 생성      색상 계산    최종 출력
```

### 2. 수정 가능한 단계들
```cpp
// 각 단계별 모딩 포인트
enum RenderStage {
    VERTEX_SHADER,      // 3D 변환, 애니메이션
    PIXEL_SHADER,       // 색상, 라이팅, 텍스처
    POST_PROCESSING,    // 블룸, DOF, 톤매핑
    UI_RENDERING,       // HUD, 메뉴 오버레이
    PRESENT             // 최종 화면 출력
};
```

### 3. DirectX Hook 포인트
```cpp
// 주요 DirectX 함수들
ID3D11DeviceContext::Draw()              // 오브젝트 렌더링
ID3D11DeviceContext::DrawIndexed()       // 인덱스 기반 렌더링  
ID3D11DeviceContext::PSSetShader()       // 픽셀 셰이더 설정
ID3D11DeviceContext::VSSetShader()       // 버텍스 셰이더 설정
IDXGISwapChain::Present()                // 화면 출력
```

## 🔧 실제 구현: ReShade 스타일 인젝터

### 기본 DirectX 11 Hook 시스템

```cpp
// D3D11Hook.h
#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>
#include <vector>
#include <memory>

class D3D11Hook {
private:
    static ID3D11Device* pDevice;
    static ID3D11DeviceContext* pContext;
    static IDXGISwapChain* pSwapChain;
    
    // 원본 함수 포인터들
    typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
    typedef void(__stdcall* PSSetShader_t)(ID3D11DeviceContext*, ID3D11PixelShader*, 
                                          ID3D11ClassInstance* const*, UINT);
    typedef void(__stdcall* Draw_t)(ID3D11DeviceContext*, UINT, UINT);
    
    static Present_t oPresent;
    static PSSetShader_t oPSSetShader;
    static Draw_t oDraw;

public:
    static bool Initialize();
    static void Shutdown();
    
    // Hook 함수들
    static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static void __stdcall hkPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader,
                                       ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances);
    static void __stdcall hkDraw(ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation);
    
private:
    static bool HookDirectX();
    static void* GetMethodAddress(int VTableIndex);
};

// D3D11Hook.cpp
#include "D3D11Hook.h"
#include "VisualEffects.h"
#include <MinHook.h>

// 전역 변수 초기화
ID3D11Device* D3D11Hook::pDevice = nullptr;
ID3D11DeviceContext* D3D11Hook::pContext = nullptr;
IDXGISwapChain* D3D11Hook::pSwapChain = nullptr;
D3D11Hook::Present_t D3D11Hook::oPresent = nullptr;
D3D11Hook::PSSetShader_t D3D11Hook::oPSSetShader = nullptr;
D3D11Hook::Draw_t D3D11Hook::oDraw = nullptr;

bool D3D11Hook::Initialize() {
    // MinHook 초기화
    if (MH_Initialize() != MH_OK) {
        return false;
    }
    
    return HookDirectX();
}

bool D3D11Hook::HookDirectX() {
    // 임시 D3D11 디바이스 생성해서 VTable 주소 획득
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetConsoleWindow();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    
    ID3D11Device* tempDevice;
    ID3D11DeviceContext* tempContext;
    IDXGISwapChain* tempSwapChain;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &tempSwapChain, &tempDevice, nullptr, &tempContext
    );
    
    if (FAILED(hr)) return false;
    
    // VTable에서 함수 주소 추출
    void** pSwapChainVTable = *reinterpret_cast<void***>(tempSwapChain);
    void** pContextVTable = *reinterpret_cast<void***>(tempContext);
    
    // Hook 설치
    MH_CreateHook(pSwapChainVTable[8], &hkPresent, (LPVOID*)&oPresent);
    MH_CreateHook(pContextVTable[9], &hkPSSetShader, (LPVOID*)&oPSSetShader);
    MH_CreateHook(pContextVTable[13], &hkDraw, (LPVOID*)&oDraw);
    
    MH_EnableHook(MH_ALL_HOOKS);
    
    // 임시 객체 해제
    tempSwapChain->Release();
    tempContext->Release();
    tempDevice->Release();
    
    return true;
}

HRESULT __stdcall D3D11Hook::hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    // 첫 번째 호출에서 실제 디바이스 저장
    if (!pDevice) {
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice);
        pDevice->GetImmediateContext(&pContext);
        D3D11Hook::pSwapChain = pSwapChain;
        
        // 시각 효과 시스템 초기화
        VisualEffects::Initialize(pDevice, pContext);
    }
    
    // 시각 효과 적용
    VisualEffects::ApplyEffects();
    
    return oPresent(pSwapChain, SyncInterval, Flags);
}

void __stdcall D3D11Hook::hkPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader,
                                       ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) {
    
    // 특정 셰이더를 커스텀 셰이더로 교체
    ID3D11PixelShader* replacementShader = VisualEffects::GetReplacementShader(pPixelShader);
    if (replacementShader) {
        pPixelShader = replacementShader;
    }
    
    return oPSSetShader(pContext, pPixelShader, ppClassInstances, NumClassInstances);
}

void __stdcall D3D11Hook::hkDraw(ID3D11DeviceContext* pContext, UINT VertexCount, UINT StartVertexLocation) {
    // 렌더링 전 상태 저장
    VisualEffects::PreRender();
    
    // 원본 Draw 호출
    oDraw(pContext, VertexCount, StartVertexLocation);
    
    // 렌더링 후 처리
    VisualEffects::PostRender();
}
```

### 시각 효과 시스템

```cpp
// VisualEffects.h
#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <unordered_map>
#include <string>

using namespace DirectX;

struct EffectParams {
    float brightness = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float gamma = 1.0f;
    XMFLOAT3 colorTint = {1.0f, 1.0f, 1.0f};
    bool enableSepia = false;
    bool enableGrayscale = false;
    float bloomStrength = 0.0f;
    float vignetteStrength = 0.0f;
};

class VisualEffects {
private:
    static ID3D11Device* pDevice;
    static ID3D11DeviceContext* pContext;
    static ID3D11RenderTargetView* pMainRTV;
    static ID3D11ShaderResourceView* pMainSRV;
    static ID3D11Texture2D* pBackBuffer;
    
    // 셰이더 리소스
    static ID3D11PixelShader* pPostProcessShader;
    static ID3D11VertexShader* pFullscreenVS;
    static ID3D11Buffer* pConstantBuffer;
    
    // 효과 파라미터
    static EffectParams currentParams;
    static std::unordered_map<ID3D11PixelShader*, ID3D11PixelShader*> shaderReplacements;

public:
    static bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    static void Shutdown();
    
    static void ApplyEffects();
    static void SetEffectParams(const EffectParams& params);
    static EffectParams& GetEffectParams() { return currentParams; }
    
    static void RegisterShaderReplacement(ID3D11PixelShader* original, ID3D11PixelShader* replacement);
    static ID3D11PixelShader* GetReplacementShader(ID3D11PixelShader* original);
    
    static void PreRender() {} // 필요시 구현
    static void PostRender() {} // 필요시 구현

private:
    static bool CreateShaders();
    static bool CreateConstantBuffer();
    static void UpdateConstantBuffer();
};

// VisualEffects.cpp
#include "VisualEffects.h"
#include <iostream>

// 전역 변수 초기화
ID3D11Device* VisualEffects::pDevice = nullptr;
ID3D11DeviceContext* VisualEffects::pContext = nullptr;
ID3D11RenderTargetView* VisualEffects::pMainRTV = nullptr;
ID3D11ShaderResourceView* VisualEffects::pMainSRV = nullptr;
ID3D11Texture2D* VisualEffects::pBackBuffer = nullptr;
ID3D11PixelShader* VisualEffects::pPostProcessShader = nullptr;
ID3D11VertexShader* VisualEffects::pFullscreenVS = nullptr;
ID3D11Buffer* VisualEffects::pConstantBuffer = nullptr;
EffectParams VisualEffects::currentParams = {};
std::unordered_map<ID3D11PixelShader*, ID3D11PixelShader*> VisualEffects::shaderReplacements;

// 포스트 프로세싱 셰이더 HLSL 코드
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
    float enableSepia;
    float enableGrayscale;
    float bloomStrength;
    float vignetteStrength;
    float padding[3];
};

Texture2D MainTexture : register(t0);
SamplerState MainSampler : register(s0);

float3 ApplyColorGrading(float3 color) {
    // 밝기 조정
    color *= brightness;
    
    // 대비 조정
    color = ((color - 0.5) * contrast) + 0.5;
    
    // 채도 조정
    float gray = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(gray.xxx, color, saturation);
    
    // 감마 보정
    color = pow(abs(color), gamma);
    
    // 색상 틴트
    color *= colorTint;
    
    return color;
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
    return color * vignette;
}

float4 main(VS_OUTPUT input) : SV_Target {
    float3 color = MainTexture.Sample(MainSampler, input.uv).rgb;
    
    // 색상 그레이딩 적용
    color = ApplyColorGrading(color);
    
    // 세피아 효과
    if (enableSepia > 0.5) {
        color = ApplySepia(color);
    }
    
    // 흑백 효과
    if (enableGrayscale > 0.5) {
        float gray = dot(color, float3(0.299, 0.587, 0.114));
        color = gray.xxx;
    }
    
    // 비네팅 효과
    if (vignetteStrength > 0.0) {
        color = ApplyVignette(color, input.uv);
    }
    
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
    
    // 전체 화면 삼각형 생성
    output.uv = float2((id << 1) & 2, id & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.pos.y = -output.pos.y;
    
    return output;
}
)";

bool VisualEffects::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    pDevice = device;
    pContext = context;
    
    if (!CreateShaders()) {
        std::cout << "셰이더 생성 실패" << std::endl;
        return false;
    }
    
    if (!CreateConstantBuffer()) {
        std::cout << "상수 버퍼 생성 실패" << std::endl;
        return false;
    }
    
    std::cout << "시각 효과 시스템 초기화 완료" << std::endl;
    return true;
}

bool VisualEffects::CreateShaders() {
    HRESULT hr;
    ID3DBlob* pShaderBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;
    
    // 픽셀 셰이더 컴파일
    hr = D3DCompile(
        postProcessShaderSource, strlen(postProcessShaderSource),
        nullptr, nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pShaderBlob, &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            std::cout << "픽셀 셰이더 컴파일 오류: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
            pErrorBlob->Release();
        }
        return false;
    }
    
    hr = pDevice->CreatePixelShader(
        pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(),
        nullptr, &pPostProcessShader
    );
    pShaderBlob->Release();
    
    if (FAILED(hr)) return false;
    
    // 버텍스 셰이더 컴파일
    hr = D3DCompile(
        fullscreenVertexShaderSource, strlen(fullscreenVertexShaderSource),
        nullptr, nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pShaderBlob, &pErrorBlob
    );
    
    if (FAILED(hr)) {
        if (pErrorBlob) {
            std::cout << "버텍스 셰이더 컴파일 오류: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
            pErrorBlob->Release();
        }
        return false;
    }
    
    hr = pDevice->CreateVertexShader(
        pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(),
        nullptr, &pFullscreenVS
    );
    pShaderBlob->Release();
    
    return SUCCEEDED(hr);
}

bool VisualEffects::CreateConstantBuffer() {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(EffectParams);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    return SUCCEEDED(pDevice->CreateBuffer(&bufferDesc, nullptr, &pConstantBuffer));
}

void VisualEffects::ApplyEffects() {
    if (!pPostProcessShader || !pFullscreenVS || !pConstantBuffer) return;
    
    // 상수 버퍼 업데이트
    UpdateConstantBuffer();
    
    // 백 버퍼 획득
    ID3D11RenderTargetView* pRTV = nullptr;
    pContext->OMGetRenderTargets(1, &pRTV, nullptr);
    
    if (pRTV) {
        // 포스트 프로세싱 적용
        pContext->VSSetShader(pFullscreenVS, nullptr, 0);
        pContext->PSSetShader(pPostProcessShader, nullptr, 0);
        pContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
        
        // 전체 화면 삼각형 그리기
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->Draw(3, 0);
        
        pRTV->Release();
    }
}

void VisualEffects::UpdateConstantBuffer() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(pContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
        memcpy(mappedResource.pData, &currentParams, sizeof(EffectParams));
        pContext->Unmap(pConstantBuffer, 0);
    }
}

void VisualEffects::SetEffectParams(const EffectParams& params) {
    currentParams = params;
}

void VisualEffects::RegisterShaderReplacement(ID3D11PixelShader* original, ID3D11PixelShader* replacement) {
    shaderReplacements[original] = replacement;
}

ID3D11PixelShader* VisualEffects::GetReplacementShader(ID3D11PixelShader* original) {
    auto it = shaderReplacements.find(original);
    return (it != shaderReplacements.end()) ? it->second : nullptr;
}
```

## 🎮 사용자 인터페이스

### ImGui 기반 실시간 조정 패널

```cpp
// EffectsGUI.h
#pragma once
#include "VisualEffects.h"
#include "imgui.h"

class EffectsGUI {
private:
    static bool showMainWindow;
    static bool showAdvancedWindow;
    static char presetName[64];

public:
    static void Initialize();
    static void Render();
    static void Shutdown();
    
private:
    static void RenderMainPanel();
    static void RenderAdvancedPanel();
    static void RenderPresetPanel();
    static void SavePreset(const std::string& name);
    static void LoadPreset(const std::string& name);
};

// EffectsGUI.cpp
#include "EffectsGUI.h"
#include <fstream>
#include <sstream>

bool EffectsGUI::showMainWindow = true;
bool EffectsGUI::showAdvancedWindow = false;
char EffectsGUI::presetName[64] = "";

void EffectsGUI::Initialize() {
    // ImGui 초기화는 이미 되어있다고 가정
}

void EffectsGUI::Render() {
    if (showMainWindow) {
        RenderMainPanel();
    }
    
    if (showAdvancedWindow) {
        RenderAdvancedPanel();
    }
}

void EffectsGUI::RenderMainPanel() {
    ImGui::Begin("Visual Effects Control", &showMainWindow);
    
    auto& params = VisualEffects::GetEffectParams();
    
    // 기본 색상 조정
    ImGui::Text("Color Grading");
    ImGui::Separator();
    ImGui::SliderFloat("Brightness", &params.brightness, 0.0f, 3.0f);
    ImGui::SliderFloat("Contrast", &params.contrast, 0.0f, 3.0f);
    ImGui::SliderFloat("Saturation", &params.saturation, 0.0f, 3.0f);
    ImGui::SliderFloat("Gamma", &params.gamma, 0.1f, 3.0f);
    
    // 색상 틴트
    ImGui::ColorEdit3("Color Tint", &params.colorTint.x);
    
    ImGui::Spacing();
    
    // 효과 옵션
    ImGui::Text("Effects");
    ImGui::Separator();
    ImGui::Checkbox("Sepia", &params.enableSepia);
    ImGui::Checkbox("Grayscale", &params.enableGrayscale);
    ImGui::SliderFloat("Bloom", &params.bloomStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Vignette", &params.vignetteStrength, 0.0f, 2.0f);
    
    ImGui::Spacing();
    
    // 프리셋 관리
    ImGui::Text("Presets");
    ImGui::Separator();
    ImGui::InputText("Preset Name", presetName, sizeof(presetName));
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        SavePreset(presetName);
    }
    
    if (ImGui::Button("Cinematic")) {
        params.brightness = 0.8f;
        params.contrast = 1.2f;
        params.saturation = 1.3f;
        params.vignetteStrength = 0.3f;
        params.enableSepia = false;
        params.enableGrayscale = false;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Vintage")) {
        params.brightness = 0.9f;
        params.contrast = 1.1f;
        params.saturation = 0.7f;
        params.enableSepia = true;
        params.vignetteStrength = 0.5f;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("High Contrast")) {
        params.brightness = 1.1f;
        params.contrast = 1.8f;
        params.saturation = 1.4f;
        params.enableSepia = false;
        params.enableGrayscale = false;
    }
    
    ImGui::Spacing();
    
    // 리셋 버튼
    if (ImGui::Button("Reset to Default")) {
        params = EffectParams{}; // 기본값으로 리셋
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Advanced...")) {
        showAdvancedWindow = true;
    }
    
    // 실시간 적용
    VisualEffects::SetEffectParams(params);
    
    ImGui::End();
}

void EffectsGUI::SavePreset(const std::string& name) {
    if (name.empty()) return;
    
    auto& params = VisualEffects::GetEffectParams();
    
    std::ofstream file("presets/" + name + ".cfg");
    if (file.is_open()) {
        file << "brightness=" << params.brightness << std::endl;
        file << "contrast=" << params.contrast << std::endl;
        file << "saturation=" << params.saturation << std::endl;
        file << "gamma=" << params.gamma << std::endl;
        file << "colorTint=" << params.colorTint.x << "," << params.colorTint.y << "," << params.colorTint.z << std::endl;
        file << "enableSepia=" << params.enableSepia << std::endl;
        file << "enableGrayscale=" << params.enableGrayscale << std::endl;
        file << "bloomStrength=" << params.bloomStrength << std::endl;
        file << "vignetteStrength=" << params.vignetteStrength << std::endl;
        file.close();
    }
}
```

## 🔬 고급 기법: 셰이더 분석 및 교체

### 1. 셰이더 디스어셈블리
```cpp
class ShaderAnalyzer {
public:
    static void AnalyzePixelShader(ID3D11PixelShader* pShader) {
        // 셰이더 바이트코드 추출
        ID3D11ShaderReflection* pReflection = nullptr;
        D3DReflect(/* 셰이더 바이트코드 */, /* 크기 */, IID_ID3D11ShaderReflection, 
                  (void**)&pReflection);
        
        if (pReflection) {
            D3D11_SHADER_DESC shaderDesc;
            pReflection->GetDesc(&shaderDesc);
            
            std::cout << "셰이더 분석:" << std::endl;
            std::cout << "- 입력 파라미터: " << shaderDesc.InputParameters << std::endl;
            std::cout << "- 상수 버퍼: " << shaderDesc.ConstantBuffers << std::endl;
            std::cout << "- 텍스처 슬롯: " << shaderDesc.BoundResources << std::endl;
            
            // 각 상수 버퍼 분석
            for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
                ID3D11ShaderReflectionConstantBuffer* pBuffer = 
                    pReflection->GetConstantBufferByIndex(i);
                
                D3D11_SHADER_BUFFER_DESC bufferDesc;
                pBuffer->GetDesc(&bufferDesc);
                
                std::cout << "상수 버퍼 " << i << ": " << bufferDesc.Name << std::endl;
                
                // 버퍼 내 변수들 분석
                for (UINT j = 0; j < bufferDesc.Variables; j++) {
                    ID3D11ShaderReflectionVariable* pVar = pBuffer->GetVariableByIndex(j);
                    D3D11_SHADER_VARIABLE_DESC varDesc;
                    pVar->GetDesc(&varDesc);
                    
                    std::cout << "  - " << varDesc.Name << std::endl;
                }
            }
            
            pReflection->Release();
        }
    }
};
```

### 2. 런타임 셰이더 컴파일
```cpp
class DynamicShaderCompiler {
private:
    static std::string LoadShaderTemplate(const std::string& templateName);
    static std::string ReplaceParameters(const std::string& source, 
                                       const std::map<std::string, std::string>& params);

public:
    static ID3D11PixelShader* CompileCustomEffect(const std::string& effectName, 
                                                 const std::map<std::string, float>& parameters) {
        // 템플릿 로드
        std::string shaderSource = LoadShaderTemplate(effectName);
        
        // 파라미터 치환
        std::map<std::string, std::string> stringParams;
        for (const auto& pair : parameters) {
            stringParams[pair.first] = std::to_string(pair.second);
        }
        
        shaderSource = ReplaceParameters(shaderSource, stringParams);
        
        // 컴파일
        ID3DBlob* pShaderBlob = nullptr;
        ID3DBlob* pErrorBlob = nullptr;
        
        HRESULT hr = D3DCompile(
            shaderSource.c_str(), shaderSource.length(),
            nullptr, nullptr, nullptr, "main", "ps_5_0",
            D3DCOMPILE_ENABLE_STRICTNESS, 0,
            &pShaderBlob, &pErrorBlob
        );
        
        if (FAILED(hr)) {
            if (pErrorBlob) {
                std::cout << "컴파일 오류: " << (char*)pErrorBlob->GetBufferPointer() << std::endl;
                pErrorBlob->Release();
            }
            return nullptr;
        }
        
        ID3D11PixelShader* pShader = nullptr;
        hr = D3D11Hook::pDevice->CreatePixelShader(
            pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(),
            nullptr, &pShader
        );
        
        pShaderBlob->Release();
        
        return SUCCEEDED(hr) ? pShader : nullptr;
    }
};
```

## 🎨 실습 과제

### 과제 1: 기본 색상 필터 (초급)
- [ ] **세피아 효과**: 갈색 톤의 빈티지 느낌
- [ ] **흑백 변환**: 명도 기반 그레이스케일  
- [ ] **색온도 조정**: 따뜻함/차가움 조절
- [ ] **채도 부스트**: 비비드한 색상 강화

### 과제 2: 고급 포스트 프로세싱 (중급)
- [ ] **블룸 효과**: 밝은 영역 발광 효과
- [ ] **깊이 기반 안개**: 거리에 따른 안개 효과  
- [ ] **모션 블러**: 움직임 잔상 효과
- [ ] **렌즈 플레어**: 광원 중심 빛 산란

### 과제 3: 실시간 셰이더 교체 (고급)
- [ ] **물 셰이더**: 파도와 반사 효과 개선
- [ ] **스킨 셰이더**: 캐릭터 피부 질감 향상
- [ ] **금속 셰이더**: 금속 재질 반사율 조정
- [ ] **파티클 효과**: 불, 연기, 마법 효과 수정

## ⚠️ 주의사항 및 한계

### 1. 성능 고려사항
```cpp
// 성능 모니터링
class PerformanceMonitor {
public:
    static void BeginFrame() {
        frameStart = std::chrono::high_resolution_clock::now();
    }
    
    static void EndFrame() {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            frameEnd - frameStart).count();
        
        // 16.67ms (60FPS) 초과 시 경고
        if (duration > 16670) {
            std::cout << "프레임 시간 초과: " << duration << "μs" << std::endl;
        }
    }
    
private:
    static std::chrono::high_resolution_clock::time_point frameStart;
};
```

### 2. 호환성 문제
```
게임 엔진별 특성:
├── DirectX 11: 표준적인 지원
├── DirectX 12: 복잡한 Hook 필요
├── Vulkan: 완전히 다른 접근 필요
├── OpenGL: 별도 구현 필요
└── 독점 엔진: 역공학 필요
```

### 3. 안전성 검증
```cpp
bool ValidateShaderReplacement(ID3D11PixelShader* original, ID3D11PixelShader* replacement) {
    // 1. 입력/출력 서명 호환성 확인
    // 2. 사용하는 리소스 슬롯 확인
    // 3. 성능 테스트 (간단한 벤치마크)
    // 4. 메모리 사용량 확인
    
    return true; // 검증 통과
}
```

## 📊 실전 디버깅 도구

### RenderDoc 통합
```cpp
class RenderDocCapture {
public:
    static void StartCapture() {
        if (rdoc_api) {
            rdoc_api->StartFrameCapture(nullptr, nullptr);
        }
    }
    
    static void EndCapture() {
        if (rdoc_api) {
            rdoc_api->EndFrameCapture(nullptr, nullptr);
        }
    }
    
private:
    static RENDERDOC_API_1_1_2* rdoc_api;
};
```

## 💡 트러블슈팅

### Q: 효과가 적용되지 않아요
```
A: 다음을 확인하세요:
1. Hook이 올바르게 설치되었는지
2. 셰이더 컴파일 오류가 없는지  
3. 상수 버퍼가 정상 업데이트되는지
4. 렌더 타겟이 올바른지
```

### Q: 게임 성능이 크게 떨어져요
```
A: 성능 최적화 방법:
1. 불필요한 효과 비활성화
2. 셰이더 복잡도 줄이기
3. 텍스처 해상도 낮추기
4. 프레임 캡처로 병목 지점 분석
```

### Q: 특정 게임에서만 크래시가 발생해요
```
A: 게임별 호환성 문제:
1. DirectX 버전 확인
2. 안티치트 시스템 간섭
3. 엔진별 특수 렌더링 파이프라인
4. 멀티스레딩 렌더링 충돌
```

---

**다음 학습**: [Scenario 04: 카메라 시스템](../scenario-04-camera-system/) | **이전**: [Scenario 02: FPS 제한 해제](../scenario-02-unlock-fps/)

**⚡ 완료 예상 시간**: 14-21일 (하루 1-2시간 투자 기준)