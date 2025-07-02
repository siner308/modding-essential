/*
 * Exercise 4: 셰이더 교체
 * 
 * 문제: 게임의 특정 셰이더를 커스텀 셰이더로 교체하는 시스템을 만드세요.
 * 
 * 학습 목표:
 * - 셰이더 인터셉션 기법
 * - HLSL 커스텀 셰이더 작성
 * - 런타임 셰이더 컴파일
 */

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <detours.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <memory>
#include <thread>
#include <atomic>
#include <codecvt>
#include <locale>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "detours.lib")

using namespace DirectX;

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

class D3D11ShaderReplacer {
private:
    static D3D11ShaderReplacer* instance;
    
    // D3D11 리소스
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    
    // 후킹 관련
    static HRESULT (STDMETHODCALLTYPE* OriginalPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT (STDMETHODCALLTYPE* OriginalCreateVertexShader)(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader);
    static HRESULT (STDMETHODCALLTYPE* OriginalCreatePixelShader)(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader);
    static void (STDMETHODCALLTYPE* OriginalVSSetShader)(ID3D11DeviceContext* pContext, ID3D11VertexShader* pVertexShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances);
    static void (STDMETHODCALLTYPE* OriginalPSSetShader)(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances);
    
    // 상태 관리
    bool initialized = false;
    bool hookInstalled = false;
    bool replacementEnabled = true;
    
    // 셰이더 관리
    struct ShaderInfo {
        std::string name;
        std::string description;
        std::vector<uint8_t> originalBytecode;
        std::vector<uint8_t> replacementBytecode;
        bool isReplaced = false;
        bool isActive = false;
        int useCount = 0;
    };
    
    struct CustomShader {
        std::string name;
        std::string source;
        std::string entryPoint;
        std::string profile;
        ID3DBlob* compiledBlob = nullptr;
    };
    
    std::unordered_map<void*, ShaderInfo> vertexShaders;
    std::unordered_map<void*, ShaderInfo> pixelShaders;
    std::unordered_map<std::string, CustomShader> customShaders;
    
    // 셰이더 패턴 매칭
    struct ShaderPattern {
        std::string name;
        std::vector<uint8_t> pattern;
        std::vector<bool> mask;
        std::string replacementShader;
    };
    
    std::vector<ShaderPattern> shaderPatterns;
    
    // 통계
    struct Statistics {
        int totalVertexShaders = 0;
        int totalPixelShaders = 0;
        int replacedVertexShaders = 0;
        int replacedPixelShaders = 0;
        int activeReplacements = 0;
    };
    
    Statistics stats;
    
    // 입력 제어
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning{false};

public:
    D3D11ShaderReplacer() {
        instance = this;
        LoadCustomShaders();
        InitializePatterns();
    }
    
    ~D3D11ShaderReplacer() {
        Cleanup();
        instance = nullptr;
    }
    
    bool InstallHook() {
        if (hookInstalled) {
            return true;
        }
        
        std::wcout << L"D3D11 셰이더 교체 후킹 시작..." << std::endl;
        
        if (!CreateTempDevice()) {
            std::wcout << L"임시 디바이스 생성 실패" << std::endl;
            return false;
        }
        
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OriginalPresent, HookedPresent);
        DetourAttach(&(PVOID&)OriginalCreateVertexShader, HookedCreateVertexShader);
        DetourAttach(&(PVOID&)OriginalCreatePixelShader, HookedCreatePixelShader);
        DetourAttach(&(PVOID&)OriginalVSSetShader, HookedVSSetShader);
        DetourAttach(&(PVOID&)OriginalPSSetShader, HookedPSSetShader);
        
        if (DetourTransactionCommit() == NO_ERROR) {
            hookInstalled = true;
            StartInputThread();
            std::wcout << L"셰이더 교체 후킹 성공" << std::endl;
            ShowControls();
            return true;
        } else {
            std::wcout << L"셰이더 교체 후킹 실패" << std::endl;
            return false;
        }
    }
    
    void UninstallHook() {
        if (!hookInstalled) {
            return;
        }
        
        StopInputThread();
        
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)OriginalPresent, HookedPresent);
        DetourDetach(&(PVOID&)OriginalCreateVertexShader, HookedCreateVertexShader);
        DetourDetach(&(PVOID&)OriginalCreatePixelShader, HookedCreatePixelShader);
        DetourDetach(&(PVOID&)OriginalVSSetShader, HookedVSSetShader);
        DetourDetach(&(PVOID&)OriginalPSSetShader, HookedPSSetShader);
        DetourTransactionCommit();
        
        hookInstalled = false;
        std::wcout << L"셰이더 교체 후킹 해제됨" << std::endl;
    }

private:
    void LoadCustomShaders() {
        // 간단한 톤 셰이더 (세피아 효과)
        customShaders["sepia"] = {
            "sepia",
            R"(
                Texture2D MainTexture : register(t0);
                SamplerState MainSampler : register(s0);
                
                struct PS_INPUT {
                    float4 pos : SV_POSITION;
                    float2 tex : TEXCOORD0;
                };
                
                float4 main(PS_INPUT input) : SV_Target {
                    float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                    
                    // 세피아 변환 행렬
                    float3 sepia;
                    sepia.r = dot(color, float3(0.393, 0.769, 0.189));
                    sepia.g = dot(color, float3(0.349, 0.686, 0.168));
                    sepia.b = dot(color, float3(0.272, 0.534, 0.131));
                    
                    return float4(sepia, 1.0);
                }
            )",
            "main",
            "ps_4_0"
        };
        
        // 엣지 검출 셰이더
        customShaders["edge_detection"] = {
            "edge_detection",
            R"(
                Texture2D MainTexture : register(t0);
                SamplerState MainSampler : register(s0);
                
                cbuffer EdgeParams : register(b0) {
                    float2 texelSize;
                    float threshold;
                    float intensity;
                };
                
                struct PS_INPUT {
                    float4 pos : SV_POSITION;
                    float2 tex : TEXCOORD0;
                };
                
                float4 main(PS_INPUT input) : SV_Target {
                    float2 uv = input.tex;
                    
                    // Sobel 필터
                    float3 tl = MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, -texelSize.y)).rgb;
                    float3 tm = MainTexture.Sample(MainSampler, uv + float2(0, -texelSize.y)).rgb;
                    float3 tr = MainTexture.Sample(MainSampler, uv + float2(texelSize.x, -texelSize.y)).rgb;
                    float3 ml = MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, 0)).rgb;
                    float3 mm = MainTexture.Sample(MainSampler, uv).rgb;
                    float3 mr = MainTexture.Sample(MainSampler, uv + float2(texelSize.x, 0)).rgb;
                    float3 bl = MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, texelSize.y)).rgb;
                    float3 bm = MainTexture.Sample(MainSampler, uv + float2(0, texelSize.y)).rgb;
                    float3 br = MainTexture.Sample(MainSampler, uv + float2(texelSize.x, texelSize.y)).rgb;
                    
                    float3 sobelX = tl * -1.0 + tr * 1.0 + ml * -2.0 + mr * 2.0 + bl * -1.0 + br * 1.0;
                    float3 sobelY = tl * -1.0 + tm * -2.0 + tr * -1.0 + bl * 1.0 + bm * 2.0 + br * 1.0;
                    
                    float3 sobel = sqrt(sobelX * sobelX + sobelY * sobelY);
                    float edge = dot(sobel, float3(0.299, 0.587, 0.114));
                    
                    if (edge > threshold) {
                        return float4(edge * intensity, edge * intensity, edge * intensity, 1.0);
                    } else {
                        return float4(mm, 1.0);
                    }
                }
            )",
            "main",
            "ps_4_0"
        };
        
        // 셀 셰이딩 (투온 스타일)
        customShaders["toon_shading"] = {
            "toon_shading",
            R"(
                Texture2D MainTexture : register(t0);
                SamplerState MainSampler : register(s0);
                
                cbuffer ToonParams : register(b0) {
                    float levels;
                    float edgeThreshold;
                    float3 edgeColor;
                };
                
                struct PS_INPUT {
                    float4 pos : SV_POSITION;
                    float2 tex : TEXCOORD0;
                    float3 normal : NORMAL;
                    float3 worldPos : WORLDPOS;
                };
                
                float4 main(PS_INPUT input) : SV_Target {
                    float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                    
                    // 색상 레벨 감소 (포스터라이제이션)
                    color = floor(color * levels) / levels;
                    
                    // 간단한 엣지 검출
                    float3 normal = normalize(input.normal);
                    float edge = 1.0 - abs(dot(normal, float3(0, 0, 1)));
                    
                    if (edge > edgeThreshold) {
                        return float4(edgeColor, 1.0);
                    }
                    
                    return float4(color, 1.0);
                }
            )",
            "main",
            "ps_4_0"
        };
        
        // 나이트 비전 효과
        customShaders["night_vision"] = {
            "night_vision",
            R"(
                Texture2D MainTexture : register(t0);
                SamplerState MainSampler : register(s0);
                
                cbuffer NightVisionParams : register(b0) {
                    float time;
                    float noiseAmount;
                    float brightness;
                    float contrast;
                };
                
                struct PS_INPUT {
                    float4 pos : SV_POSITION;
                    float2 tex : TEXCOORD0;
                };
                
                float random(float2 uv) {
                    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
                }
                
                float4 main(PS_INPUT input) : SV_Target {
                    float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                    
                    // 그레이스케일로 변환
                    float gray = dot(color, float3(0.299, 0.587, 0.114));
                    
                    // 밝기와 대비 조정
                    gray = ((gray - 0.5) * contrast + 0.5) * brightness;
                    
                    // 녹색 틴트
                    float3 nightVision = float3(gray * 0.2, gray, gray * 0.2);
                    
                    // 노이즈 추가
                    float noise = random(input.tex + time) * noiseAmount;
                    nightVision += noise;
                    
                    // 비네팅 효과
                    float2 center = input.tex - 0.5;
                    float vignette = 1.0 - smoothstep(0.3, 0.8, length(center));
                    nightVision *= vignette;
                    
                    return float4(nightVision, 1.0);
                }
            )",
            "main",
            "ps_4_0"
        };
        
        // 모든 커스텀 셰이더 컴파일
        CompileCustomShaders();
    }
    
    void InitializePatterns() {
        // 일반적인 라이팅 셰이더 패턴 (예시)
        // 실제로는 리버스 엔지니어링을 통해 패턴을 찾아야 함
        
        ShaderPattern lightingPattern;
        lightingPattern.name = "standard_lighting";
        lightingPattern.pattern = {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08}; // 예시 바이트코드 패턴
        lightingPattern.mask = {true, true, true, true, true, true, true};
        lightingPattern.replacementShader = "toon_shading";
        shaderPatterns.push_back(lightingPattern);
        
        ShaderPattern postProcessPattern;
        postProcessPattern.name = "post_process";
        postProcessPattern.pattern = {0x89, 0x05, 0x00, 0x00, 0x00, 0x00}; // 예시 패턴
        postProcessPattern.mask = {true, true, false, false, false, false};
        postProcessPattern.replacementShader = "sepia";
        shaderPatterns.push_back(postProcessPattern);
    }
    
    void CompileCustomShaders() {
        for (auto& pair : customShaders) {
            CustomShader& shader = pair.second;
            
            ID3DBlob* errorBlob = nullptr;
            HRESULT hr = D3DCompile(
                shader.source.c_str(),
                shader.source.length(),
                nullptr,
                nullptr,
                nullptr,
                shader.entryPoint.c_str(),
                shader.profile.c_str(),
                D3DCOMPILE_ENABLE_STRICTNESS,
                0,
                &shader.compiledBlob,
                &errorBlob
            );
            
            if (FAILED(hr)) {
                std::wcerr << L"셰이더 컴파일 실패: " << StringToWString(shader.name) << std::endl;
                if (errorBlob) {
                    std::wcerr << L"오류: " << StringToWString(reinterpret_cast<char*>(errorBlob->GetBufferPointer())) << std::endl;
                    errorBlob->Release();
                }
            } else {
                std::wcout << L"셰이더 컴파일 성공: " << StringToWString(shader.name) << std::endl;
            }
        }
    }
    
    bool CreateTempDevice() {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = 800;
        swapChainDesc.BufferDesc.Height = 600;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = GetDesktopWindow();
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;
        
        D3D_FEATURE_LEVEL featureLevel;
        ID3D11Device* tempDevice = nullptr;
        ID3D11DeviceContext* tempContext = nullptr;
        IDXGISwapChain* tempSwapChain = nullptr;
        
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &swapChainDesc, &tempSwapChain, &tempDevice, &featureLevel, &tempContext);
        
        if (SUCCEEDED(hr)) {
            // VTable에서 함수 주소 추출
            void** swapChainVTable = *reinterpret_cast<void***>(tempSwapChain);
            void** deviceVTable = *reinterpret_cast<void***>(tempDevice);
            void** contextVTable = *reinterpret_cast<void***>(tempContext);
            
            OriginalPresent = reinterpret_cast<decltype(OriginalPresent)>(swapChainVTable[8]);
            OriginalCreateVertexShader = reinterpret_cast<decltype(OriginalCreateVertexShader)>(deviceVTable[12]);
            OriginalCreatePixelShader = reinterpret_cast<decltype(OriginalCreatePixelShader)>(deviceVTable[15]);
            OriginalVSSetShader = reinterpret_cast<decltype(OriginalVSSetShader)>(contextVTable[11]);
            OriginalPSSetShader = reinterpret_cast<decltype(OriginalPSSetShader)>(contextVTable[9]);
            
            tempContext->Release();
            tempDevice->Release();
            tempSwapChain->Release();
            
            return true;
        }
        
        return false;
    }
    
    static HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (instance) {
            instance->OnPresent(pSwapChain);
        }
        return OriginalPresent(pSwapChain, SyncInterval, Flags);
    }
    
    static HRESULT STDMETHODCALLTYPE HookedCreateVertexShader(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader) {
        if (instance) {
            return instance->OnCreateVertexShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
        }
        return OriginalCreateVertexShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
    }
    
    static HRESULT STDMETHODCALLTYPE HookedCreatePixelShader(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader) {
        if (instance) {
            return instance->OnCreatePixelShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
        }
        return OriginalCreatePixelShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
    }
    
    static void STDMETHODCALLTYPE HookedVSSetShader(ID3D11DeviceContext* pContext, ID3D11VertexShader* pVertexShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances) {
        if (instance) {
            instance->OnVSSetShader(pContext, pVertexShader, ppClassInstances, NumClassInstances);
        } else {
            OriginalVSSetShader(pContext, pVertexShader, ppClassInstances, NumClassInstances);
        }
    }
    
    static void STDMETHODCALLTYPE HookedPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances) {
        if (instance) {
            instance->OnPSSetShader(pContext, pPixelShader, ppClassInstances, NumClassInstances);
        } else {
            OriginalPSSetShader(pContext, pPixelShader, ppClassInstances, NumClassInstances);
        }
    }
    
    void OnPresent(IDXGISwapChain* pSwapChain) {
        if (!initialized) {
            if (InitializeResources(pSwapChain)) {
                initialized = true;
                std::wcout << L"셰이더 교체 시스템 초기화 완료" << std::endl;
            }
        }
    }
    
    HRESULT OnCreateVertexShader(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader) {
        stats.totalVertexShaders++;
        
        // 원본 셰이더 생성
        HRESULT hr = OriginalCreateVertexShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
        
        if (SUCCEEDED(hr) && ppVertexShader && *ppVertexShader) {
            // 셰이더 정보 저장
            ShaderInfo info;
            info.name = "VertexShader_" + std::to_string(stats.totalVertexShaders);
            info.description = "Intercepted vertex shader";
            info.originalBytecode.assign(static_cast<const uint8_t*>(pShaderBytecode), 
                                       static_cast<const uint8_t*>(pShaderBytecode) + BytecodeLength);
            
            vertexShaders[*ppVertexShader] = info;
            
            // 패턴 매칭 및 교체 로직은 여기에 추가
            // (실제 구현에서는 바이트코드 분석이 필요)
        }
        
        return hr;
    }
    
    HRESULT OnCreatePixelShader(ID3D11Device* pDevice, const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader) {
        stats.totalPixelShaders++;
        
        // 바이트코드 분석
        std::vector<uint8_t> bytecode(static_cast<const uint8_t*>(pShaderBytecode), 
                                    static_cast<const uint8_t*>(pShaderBytecode) + BytecodeLength);
        
        // 교체 대상 셰이더인지 확인
        std::string replacementShader = CheckForReplacement(bytecode);
        
        if (!replacementShader.empty() && replacementEnabled) {
            // 커스텀 셰이더로 교체
            auto it = customShaders.find(replacementShader);
            if (it != customShaders.end() && it->second.compiledBlob) {
                std::wcout << L"셰이더 교체: " << StringToWString(replacementShader) << std::endl;
                
                HRESULT hr = OriginalCreatePixelShader(pDevice, 
                    it->second.compiledBlob->GetBufferPointer(),
                    it->second.compiledBlob->GetBufferSize(),
                    pClassLinkage, ppPixelShader);
                
                if (SUCCEEDED(hr)) {
                    stats.replacedPixelShaders++;
                    
                    // 교체된 셰이더 정보 저장
                    ShaderInfo info;
                    info.name = replacementShader;
                    info.description = "Replaced with custom shader: " + replacementShader;
                    info.originalBytecode = bytecode;
                    info.isReplaced = true;
                    
                    pixelShaders[*ppVertexShader] = info;
                }
                
                return hr;
            }
        }
        
        // 원본 셰이더 생성
        HRESULT hr = OriginalCreatePixelShader(pDevice, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
        
        if (SUCCEEDED(hr) && ppPixelShader && *ppPixelShader) {
            // 셰이더 정보 저장
            ShaderInfo info;
            info.name = "PixelShader_" + std::to_string(stats.totalPixelShaders);
            info.description = "Intercepted pixel shader";
            info.originalBytecode = bytecode;
            
            pixelShaders[*ppPixelShader] = info;
        }
        
        return hr;
    }
    
    void OnVSSetShader(ID3D11DeviceContext* pContext, ID3D11VertexShader* pVertexShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances) {
        if (pVertexShader) {
            auto it = vertexShaders.find(pVertexShader);
            if (it != vertexShaders.end()) {
                it->second.isActive = true;
                it->second.useCount++;
            }
        }
        
        OriginalVSSetShader(pContext, pVertexShader, ppClassInstances, NumClassInstances);
    }
    
    void OnPSSetShader(ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, ID3D11ClassLinkage* const* ppClassInstances, UINT NumClassInstances) {
        if (pPixelShader) {
            auto it = pixelShaders.find(pPixelShader);
            if (it != pixelShaders.end()) {
                it->second.isActive = true;
                it->second.useCount++;
                if (it->second.isReplaced) {
                    stats.activeReplacements++;
                }
            }
        }
        
        OriginalPSSetShader(pContext, pPixelShader, ppClassInstances, NumClassInstances);
    }
    
    std::string CheckForReplacement(const std::vector<uint8_t>& bytecode) {
        for (const auto& pattern : shaderPatterns) {
            if (MatchesPattern(bytecode, pattern)) {
                return pattern.replacementShader;
            }
        }
        
        // 간단한 휴리스틱 기반 교체
        // (실제로는 더 정교한 분석이 필요)
        if (bytecode.size() > 1000 && bytecode.size() < 5000) {
            // 중간 크기 셰이더는 sepia로 교체
            return "sepia";
        }
        
        return "";
    }
    
    bool MatchesPattern(const std::vector<uint8_t>& bytecode, const ShaderPattern& pattern) {
        if (bytecode.size() < pattern.pattern.size()) {
            return false;
        }
        
        for (size_t i = 0; i <= bytecode.size() - pattern.pattern.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < pattern.pattern.size(); ++j) {
                if (pattern.mask[j] && bytecode[i + j] != pattern.pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        
        return false;
    }
    
    bool InitializeResources(IDXGISwapChain* pSwapChain) {
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));
        if (FAILED(hr)) return false;
        
        device->GetImmediateContext(&context);
        swapChain = pSwapChain;
        swapChain->AddRef();
        
        return true;
    }
    
    void StartInputThread() {
        inputThreadRunning = true;
        inputThread = std::thread(&D3D11ShaderReplacer::InputThreadFunc, this);
    }
    
    void StopInputThread() {
        inputThreadRunning = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
    }
    
    void InputThreadFunc() {
        while (inputThreadRunning) {
            if (GetAsyncKeyState(VK_F1) & 0x8000) {
                replacementEnabled = !replacementEnabled;
                std::wcout << L"셰이더 교체: " << (replacementEnabled ? L"켜짐" : L"꺼짐") << std::endl;
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F2) & 0x8000) {
                ShowStatistics();
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F3) & 0x8000) {
                ExportShaderInfo();
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F4) & 0x8000) {
                ListActiveShaders();
                Sleep(200);
            }
            
            Sleep(50);
        }
    }
    
    void ShowControls() {
        std::wcout << L"\n=== 셰이더 교체 컨트롤 ===" << std::endl;
        std::wcout << L"F1: 셰이더 교체 켜기/끄기" << std::endl;
        std::wcout << L"F2: 통계 보기" << std::endl;
        std::wcout << L"F3: 셰이더 정보 내보내기" << std::endl;
        std::wcout << L"F4: 활성 셰이더 목록" << std::endl;
        std::wcout << L"==========================\n" << std::endl;
    }
    
    void ShowStatistics() {
        std::wcout << L"\n=== 셰이더 교체 통계 ===" << std::endl;
        std::wcout << L"총 버텍스 셰이더: " << stats.totalVertexShaders << std::endl;
        std::wcout << L"총 픽셀 셰이더: " << stats.totalPixelShaders << std::endl;
        std::wcout << L"교체된 버텍스 셰이더: " << stats.replacedVertexShaders << std::endl;
        std::wcout << L"교체된 픽셀 셰이더: " << stats.replacedPixelShaders << std::endl;
        std::wcout << L"현재 활성 교체: " << stats.activeReplacements << std::endl;
        std::wcout << L"교체 상태: " << (replacementEnabled ? L"활성화" : L"비활성화") << std::endl;
        std::wcout << L"=======================\n" << std::endl;
    }
    
    void ExportShaderInfo() {
        std::ofstream file("shader_info.txt");
        if (!file.is_open()) {
            std::wcout << L"파일 생성 실패" << std::endl;
            return;
        }
        
        file << "=== Shader Replacement Report ===" << std::endl;
        file << "Total Vertex Shaders: " << stats.totalVertexShaders << std::endl;
        file << "Total Pixel Shaders: " << stats.totalPixelShaders << std::endl;
        file << "Replaced Pixel Shaders: " << stats.replacedPixelShaders << std::endl;
        file << std::endl;
        
        file << "=== Pixel Shader Details ===" << std::endl;
        for (const auto& pair : pixelShaders) {
            const ShaderInfo& info = pair.second;
            file << "Name: " << info.name << std::endl;
            file << "Description: " << info.description << std::endl;
            file << "Replaced: " << (info.isReplaced ? "Yes" : "No") << std::endl;
            file << "Use Count: " << info.useCount << std::endl;
            file << "Bytecode Size: " << info.originalBytecode.size() << " bytes" << std::endl;
            file << "---" << std::endl;
        }
        
        file.close();
        std::wcout << L"셰이더 정보가 shader_info.txt에 저장되었습니다." << std::endl;
    }
    
    void ListActiveShaders() {
        std::wcout << L"\n=== 활성 셰이더 목록 ===" << std::endl;
        
        int activeCount = 0;
        for (const auto& pair : pixelShaders) {
            const ShaderInfo& info = pair.second;
            if (info.isActive) {
                std::wcout << L"- " << StringToWString(info.name);
                if (info.isReplaced) {
                    std::wcout << L" [교체됨]";
                }
                std::wcout << L" (사용횟수: " << info.useCount << L")" << std::endl;
                activeCount++;
            }
        }
        
        if (activeCount == 0) {
            std::wcout << L"활성 셰이더가 없습니다." << std::endl;
        }
        
        std::wcout << L"==================\n" << std::endl;
    }
    
    void Cleanup() {
        StopInputThread();
        
        // 커스텀 셰이더 정리
        for (auto& pair : customShaders) {
            if (pair.second.compiledBlob) {
                pair.second.compiledBlob->Release();
                pair.second.compiledBlob = nullptr;
            }
        }
        
        if (context) { context->Release(); context = nullptr; }
        if (device) { device->Release(); device = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    }
};

// 정적 멤버 정의
D3D11ShaderReplacer* D3D11ShaderReplacer::instance = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11ShaderReplacer::OriginalPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11ShaderReplacer::OriginalCreateVertexShader)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11VertexShader**) = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11ShaderReplacer::OriginalCreatePixelShader)(ID3D11Device*, const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11PixelShader**) = nullptr;
void (STDMETHODCALLTYPE* D3D11ShaderReplacer::OriginalVSSetShader)(ID3D11DeviceContext*, ID3D11VertexShader*, ID3D11ClassLinkage* const*, UINT) = nullptr;
void (STDMETHODCALLTYPE* D3D11ShaderReplacer::OriginalPSSetShader)(ID3D11DeviceContext*, ID3D11PixelShader*, ID3D11ClassLinkage* const*, UINT) = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static D3D11ShaderReplacer* replacer = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"D3D11 셰이더 교체 DLL 로드됨" << std::endl;
            
            replacer = new D3D11ShaderReplacer();
            if (!replacer->InstallHook()) {
                delete replacer;
                replacer = nullptr;
                std::wcout << L"셰이더 교체 설치 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (replacer) {
                replacer->UninstallHook();
                delete replacer;
                replacer = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}