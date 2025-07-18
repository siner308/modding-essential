/*
 * Exercise 5: 포스트 프로세싱 효과
 * 
 * 문제: 블룸, 엣지 디텍션, 모션 블러 중 하나를 구현하세요.
 * 
 * 학습 목표:
 * - 고급 포스트 프로세싱 기법
 * - 멀티패스 렌더링
 * - GPU 기반 이미지 처리
 */

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <detours.h>
#include <iostream>
#include <string>
#include <vector>
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

class D3D11PostProcessor {
private:
    static D3D11PostProcessor* instance;
    
    // D3D11 리소스
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;
    
    // 포스트 프로세싱 리소스
    ID3D11Texture2D* tempTexture = nullptr;
    ID3D11RenderTargetView* tempRTV = nullptr;
    ID3D11ShaderResourceView* tempSRV = nullptr;
    
    // 블룸 효과용 리소스
    ID3D11Texture2D* bloomTexture = nullptr;
    ID3D11RenderTargetView* bloomRTV = nullptr;
    ID3D11ShaderResourceView* bloomSRV = nullptr;
    ID3D11Texture2D* brightTexture = nullptr;
    ID3D11RenderTargetView* brightRTV = nullptr;
    ID3D11ShaderResourceView* brightSRV = nullptr;
    
    // 블러 효과용 리소스 (다중 패스)
    static const int BLUR_PASSES = 3;
    ID3D11Texture2D* blurTextures[BLUR_PASSES];
    ID3D11RenderTargetView* blurRTVs[BLUR_PASSES];
    ID3D11ShaderResourceView* blurSRVs[BLUR_PASSES];
    
    // 모션 블러용 리소스
    ID3D11Texture2D* velocityTexture = nullptr;
    ID3D11RenderTargetView* velocityRTV = nullptr;
    ID3D11ShaderResourceView* velocitySRV = nullptr;
    ID3D11Texture2D* previousFrameTexture = nullptr;
    ID3D11ShaderResourceView* previousFrameSRV = nullptr;
    
    // 셰이더 리소스
    ID3D11VertexShader* fullscreenVS = nullptr;
    ID3D11PixelShader* brightPassPS = nullptr;
    ID3D11PixelShader* gaussianBlurHPS = nullptr;
    ID3D11PixelShader* gaussianBlurVPS = nullptr;
    ID3D11PixelShader* bloomCombinePS = nullptr;
    ID3D11PixelShader* motionBlurPS = nullptr;
    ID3D11PixelShader* edgeDetectionPS = nullptr;
    ID3D11PixelShader* finalCombinePS = nullptr;
    
    // 버퍼와 상태
    ID3D11Buffer* fullscreenVB = nullptr;
    ID3D11Buffer* postProcessCB = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11SamplerState* linearSampler = nullptr;
    ID3D11SamplerState* pointSampler = nullptr;
    ID3D11BlendState* additiveBlend = nullptr;
    ID3D11BlendState* alphaBlend = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;
    ID3D11DepthStencilState* depthStencilState = nullptr;
    
    // 후킹 관련
    static HRESULT (STDMETHODCALLTYPE* OriginalPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT (STDMETHODCALLTYPE* OriginalResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    
    // 상태 관리
    bool initialized = false;
    bool hookInstalled = false;
    UINT screenWidth = 0;
    UINT screenHeight = 0;
    
    // 효과 설정
    enum class EffectType {
        None,
        Bloom,
        MotionBlur,
        EdgeDetection,
        Combined
    };
    
    EffectType currentEffect = EffectType::Bloom;
    
    struct PostProcessParams {
        // 공통 파라미터
        float intensity = 1.0f;
        float threshold = 0.5f;
        float exposure = 1.0f;
        float gamma = 2.2f;
        
        // 블룸 파라미터
        float bloomIntensity = 0.8f;
        float bloomThreshold = 0.6f;
        float bloomRadius = 1.0f;
        float bloomSaturation = 1.2f;
        
        // 모션 블러 파라미터
        float motionBlurStrength = 0.5f;
        int motionBlurSamples = 16;
        float velocityScale = 1.0f;
        float maxBlurRadius = 32.0f;
        
        // 엣지 디텍션 파라미터
        float edgeThreshold = 0.1f;
        float edgeIntensity = 1.0f;
        XMFLOAT3 edgeColor = {1.0f, 1.0f, 1.0f};
        float padding = 0.0f;
    };
    
    PostProcessParams params;
    std::atomic<bool> paramsChanged{false};
    
    // 프레임 정보
    XMMATRIX previousViewProjection;
    XMMATRIX currentViewProjection;
    bool hasPreviousFrame = false;
    
    // 입력 제어
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning{false};
    
    // 렌더링 구조체
    struct FullscreenVertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
    };

public:
    D3D11PostProcessor() {
        instance = this;
        // 이전 프레임 매트릭스 초기화
        previousViewProjection = XMMatrixIdentity();
        currentViewProjection = XMMatrixIdentity();
    }
    
    ~D3D11PostProcessor() {
        Cleanup();
        instance = nullptr;
    }
    
    bool InstallHook() {
        if (hookInstalled) {
            return true;
        }
        
        std::wcout << L"D3D11 포스트 프로세서 후킹 시작..." << std::endl;
        
        if (!CreateTempDevice()) {
            std::wcout << L"임시 디바이스 생성 실패" << std::endl;
            return false;
        }
        
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OriginalPresent, HookedPresent);
        DetourAttach(&(PVOID&)OriginalResizeBuffers, HookedResizeBuffers);
        
        if (DetourTransactionCommit() == NO_ERROR) {
            hookInstalled = true;
            StartInputThread();
            std::wcout << L"포스트 프로세서 후킹 성공" << std::endl;
            ShowControls();
            return true;
        } else {
            std::wcout << L"포스트 프로세서 후킹 실패" << std::endl;
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
        DetourDetach(&(PVOID&)OriginalResizeBuffers, HookedResizeBuffers);
        DetourTransactionCommit();
        
        hookInstalled = false;
        std::wcout << L"포스트 프로세서 후킹 해제됨" << std::endl;
    }

private:
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
            void** swapChainVTable = *reinterpret_cast<void***>(tempSwapChain);
            OriginalPresent = reinterpret_cast<decltype(OriginalPresent)>(swapChainVTable[8]);
            OriginalResizeBuffers = reinterpret_cast<decltype(OriginalResizeBuffers)>(swapChainVTable[13]);
            
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
    
    static HRESULT STDMETHODCALLTYPE HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        if (instance) {
            instance->OnResizeBuffers();
        }
        return OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }
    
    void OnPresent(IDXGISwapChain* pSwapChain) {
        if (!initialized) {
            if (InitializeResources(pSwapChain)) {
                initialized = true;
                std::wcout << L"포스트 프로세서 초기화 완료" << std::endl;
            } else {
                return;
            }
        }
        
        if (currentEffect != EffectType::None) {
            ApplyPostProcessing();
        }
    }
    
    void OnResizeBuffers() {
        CleanupRenderTargets();
        initialized = false;
    }
    
    bool InitializeResources(IDXGISwapChain* pSwapChain) {
        // SwapChain에서 디바이스와 컨텍스트 획득
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));
        if (FAILED(hr)) return false;
        
        device->GetImmediateContext(&context);
        swapChain = pSwapChain;
        swapChain->AddRef();
        
        // 화면 크기 획득
        DXGI_SWAP_CHAIN_DESC desc;
        swapChain->GetDesc(&desc);
        screenWidth = desc.BufferDesc.Width;
        screenHeight = desc.BufferDesc.Height;
        
        // 백버퍼와 렌더 타겟 뷰 생성
        if (!CreateRenderTargets()) return false;
        
        // 셰이더 생성
        if (!CreateShaders()) return false;
        
        // 렌더링 상태 생성
        if (!CreateRenderStates()) return false;
        
        // 풀스크린 쿼드 생성
        if (!CreateFullscreenQuad()) return false;
        
        return true;
    }
    
    bool CreateRenderTargets() {
        // 메인 백버퍼 렌더 타겟
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(backBuffer, nullptr, &mainRenderTargetView);
        backBuffer->Release();
        if (FAILED(hr)) return false;
        
        // 임시 텍스처 (원본 백버퍼 복사용)
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = screenWidth;
        textureDesc.Height = screenHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR 지원
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        
        hr = device->CreateTexture2D(&textureDesc, nullptr, &tempTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(tempTexture, nullptr, &tempRTV);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(tempTexture, nullptr, &tempSRV);
        if (FAILED(hr)) return false;
        
        // 블룸용 텍스처들 (1/4 해상도)
        textureDesc.Width = screenWidth / 4;
        textureDesc.Height = screenHeight / 4;
        
        hr = device->CreateTexture2D(&textureDesc, nullptr, &bloomTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(bloomTexture, nullptr, &bloomRTV);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(bloomTexture, nullptr, &bloomSRV);
        if (FAILED(hr)) return false;
        
        // 브라이트 패스용 텍스처
        hr = device->CreateTexture2D(&textureDesc, nullptr, &brightTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(brightTexture, nullptr, &brightRTV);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(brightTexture, nullptr, &brightSRV);
        if (FAILED(hr)) return false;
        
        // 블러용 텍스처들 (점점 작아지는 해상도)
        for (int i = 0; i < BLUR_PASSES; ++i) {
            textureDesc.Width = (screenWidth / 4) >> i;
            textureDesc.Height = (screenHeight / 4) >> i;
            
            hr = device->CreateTexture2D(&textureDesc, nullptr, &blurTextures[i]);
            if (FAILED(hr)) return false;
            
            hr = device->CreateRenderTargetView(blurTextures[i], nullptr, &blurRTVs[i]);
            if (FAILED(hr)) return false;
            
            hr = device->CreateShaderResourceView(blurTextures[i], nullptr, &blurSRVs[i]);
            if (FAILED(hr)) return false;
        }
        
        // 모션 블러용 텍스처들
        textureDesc.Width = screenWidth;
        textureDesc.Height = screenHeight;
        textureDesc.Format = DXGI_FORMAT_R16G16_FLOAT; // 벨로시티용
        
        hr = device->CreateTexture2D(&textureDesc, nullptr, &velocityTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(velocityTexture, nullptr, &velocityRTV);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(velocityTexture, nullptr, &velocitySRV);
        if (FAILED(hr)) return false;
        
        // 이전 프레임 텍스처
        textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        
        hr = device->CreateTexture2D(&textureDesc, nullptr, &previousFrameTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(previousFrameTexture, nullptr, &previousFrameSRV);
        if (FAILED(hr)) return false;
        
        return true;
    }
    
    void CleanupRenderTargets() {
        if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
        if (tempTexture) { tempTexture->Release(); tempTexture = nullptr; }
        if (tempRTV) { tempRTV->Release(); tempRTV = nullptr; }
        if (tempSRV) { tempSRV->Release(); tempSRV = nullptr; }
        if (bloomTexture) { bloomTexture->Release(); bloomTexture = nullptr; }
        if (bloomRTV) { bloomRTV->Release(); bloomRTV = nullptr; }
        if (bloomSRV) { bloomSRV->Release(); bloomSRV = nullptr; }
        if (brightTexture) { brightTexture->Release(); brightTexture = nullptr; }
        if (brightRTV) { brightRTV->Release(); brightRTV = nullptr; }
        if (brightSRV) { brightSRV->Release(); brightSRV = nullptr; }
        
        for (int i = 0; i < BLUR_PASSES; ++i) {
            if (blurTextures[i]) { blurTextures[i]->Release(); blurTextures[i] = nullptr; }
            if (blurRTVs[i]) { blurRTVs[i]->Release(); blurRTVs[i] = nullptr; }
            if (blurSRVs[i]) { blurSRVs[i]->Release(); blurSRVs[i] = nullptr; }
        }
        
        if (velocityTexture) { velocityTexture->Release(); velocityTexture = nullptr; }
        if (velocityRTV) { velocityRTV->Release(); velocityRTV = nullptr; }
        if (velocitySRV) { velocitySRV->Release(); velocitySRV = nullptr; }
        if (previousFrameTexture) { previousFrameTexture->Release(); previousFrameTexture = nullptr; }
        if (previousFrameSRV) { previousFrameSRV->Release(); previousFrameSRV = nullptr; }
    }
    
    bool CreateShaders() {
        // 풀스크린 버텍스 셰이더
        const char* fullscreenVSSource = R"(
            struct VS_INPUT {
                float3 pos : POSITION;
                float2 tex : TEXCOORD0;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            PS_INPUT main(VS_INPUT input) {
                PS_INPUT output;
                output.pos = float4(input.pos, 1.0f);
                output.tex = input.tex;
                return output;
            }
        )";
        
        // 브라이트 패스 셰이더 (블룸용)
        const char* brightPassPSSource = R"(
            Texture2D MainTexture : register(t0);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                float motionBlurStrength;
                int motionBlurSamples;
                float velocityScale;
                float maxBlurRadius;
                float edgeThreshold;
                float edgeIntensity;
                float3 edgeColor;
                float padding;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            float4 main(PS_INPUT input) : SV_Target {
                float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                
                // 노출 조정
                color *= exposure;
                
                // 휘도 계산
                float luminance = dot(color, float3(0.299, 0.587, 0.114));
                
                // 임계값 적용
                float brightness = max(luminance - bloomThreshold, 0.0);
                brightness /= (1.0 + brightness); // 톤매핑
                
                return float4(color * brightness * bloomIntensity, 1.0);
            }
        )";
        
        // 가우시안 블러 셰이더 (수평)
        const char* gaussianBlurHPSSource = R"(
            Texture2D MainTexture : register(t0);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                // ... other params
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            static const float weights[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };
            
            float4 main(PS_INPUT input) : SV_Target {
                float2 texelSize;
                MainTexture.GetDimensions(texelSize.x, texelSize.y);
                texelSize = bloomRadius / texelSize;
                
                float3 result = MainTexture.Sample(MainSampler, input.tex).rgb * weights[0];
                
                for (int i = 1; i < 5; ++i) {
                    float2 offset = float2(texelSize.x * i, 0.0);
                    result += MainTexture.Sample(MainSampler, input.tex + offset).rgb * weights[i];
                    result += MainTexture.Sample(MainSampler, input.tex - offset).rgb * weights[i];
                }
                
                return float4(result, 1.0);
            }
        )";
        
        // 가우시안 블러 셰이더 (수직)
        const char* gaussianBlurVPSSource = R"(
            Texture2D MainTexture : register(t0);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                // ... other params
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            static const float weights[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };
            
            float4 main(PS_INPUT input) : SV_Target {
                float2 texelSize;
                MainTexture.GetDimensions(texelSize.x, texelSize.y);
                texelSize = bloomRadius / texelSize;
                
                float3 result = MainTexture.Sample(MainSampler, input.tex).rgb * weights[0];
                
                for (int i = 1; i < 5; ++i) {
                    float2 offset = float2(0.0, texelSize.y * i);
                    result += MainTexture.Sample(MainSampler, input.tex + offset).rgb * weights[i];
                    result += MainTexture.Sample(MainSampler, input.tex - offset).rgb * weights[i];
                }
                
                return float4(result, 1.0);
            }
        )";
        
        // 블룸 합성 셰이더
        const char* bloomCombinePSSource = R"(
            Texture2D MainTexture : register(t0);
            Texture2D BloomTexture : register(t1);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                // ... other params
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            float4 main(PS_INPUT input) : SV_Target {
                float3 sceneColor = MainTexture.Sample(MainSampler, input.tex).rgb;
                float3 bloomColor = BloomTexture.Sample(MainSampler, input.tex).rgb;
                
                // 블룸 채도 조정
                float bloomLuminance = dot(bloomColor, float3(0.299, 0.587, 0.114));
                bloomColor = lerp(bloomLuminance.xxx, bloomColor, bloomSaturation);
                
                // 블룸 합성 (스크린 블렌드 모드)
                float3 result = sceneColor + bloomColor * bloomIntensity;
                
                // 톤매핑 (ACES)
                result = (result * (2.51 * result + 0.03)) / (result * (2.43 * result + 0.59) + 0.14);
                
                // 감마 보정
                result = pow(abs(result), 1.0 / gamma);
                
                return float4(result, 1.0);
            }
        )";
        
        // 모션 블러 셰이더
        const char* motionBlurPSSource = R"(
            Texture2D MainTexture : register(t0);
            Texture2D VelocityTexture : register(t1);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                float motionBlurStrength;
                int motionBlurSamples;
                float velocityScale;
                float maxBlurRadius;
                // ... other params
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            float4 main(PS_INPUT input) : SV_Target {
                float2 velocity = VelocityTexture.Sample(MainSampler, input.tex).xy * velocityScale;
                
                // 최대 블러 반지름 제한
                float velocityLength = length(velocity);
                if (velocityLength > maxBlurRadius) {
                    velocity = normalize(velocity) * maxBlurRadius;
                }
                
                float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                
                // 모션 블러 샘플링
                for (int i = 1; i < motionBlurSamples; ++i) {
                    float2 offset = velocity * (float(i) / float(motionBlurSamples - 1) - 0.5) * motionBlurStrength;
                    color += MainTexture.Sample(MainSampler, input.tex + offset).rgb;
                }
                
                color /= float(motionBlurSamples);
                
                return float4(color, 1.0);
            }
        )";
        
        // 엣지 디텍션 셰이더
        const char* edgeDetectionPSSource = R"(
            Texture2D MainTexture : register(t0);
            SamplerState MainSampler : register(s0);
            
            cbuffer PostProcessParams : register(b0) {
                float intensity;
                float threshold;
                float exposure;
                float gamma;
                float bloomIntensity;
                float bloomThreshold;
                float bloomRadius;
                float bloomSaturation;
                float motionBlurStrength;
                int motionBlurSamples;
                float velocityScale;
                float maxBlurRadius;
                float edgeThreshold;
                float edgeIntensity;
                float3 edgeColor;
                float padding;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            float4 main(PS_INPUT input) : SV_Target {
                float2 texelSize;
                MainTexture.GetDimensions(texelSize.x, texelSize.y);
                texelSize = 1.0 / texelSize;
                
                // Sobel 엣지 디텍션
                float3 tl = MainTexture.Sample(MainSampler, input.tex + float2(-texelSize.x, -texelSize.y)).rgb;
                float3 tm = MainTexture.Sample(MainSampler, input.tex + float2(0, -texelSize.y)).rgb;
                float3 tr = MainTexture.Sample(MainSampler, input.tex + float2(texelSize.x, -texelSize.y)).rgb;
                float3 ml = MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, 0)).rgb;
                float3 mm = MainTexture.Sample(MainSampler, input.tex).rgb;
                float3 mr = MainTexture.Sample(MainSampler, uv + float2(texelSize.x, 0)).rgb;
                float3 bl = MainTexture.Sample(MainSampler, uv + float2(-texelSize.x, texelSize.y)).rgb;
                float3 bm = MainTexture.Sample(MainSampler, uv + float2(0, texelSize.y)).rgb;
                float3 br = MainTexture.Sample(MainSampler, uv + float2(texelSize.x, texelSize.y)).rgb;
                
                float3 sobelX = tl * -1.0 + tr * 1.0 + ml * -2.0 + mr * 2.0 + bl * -1.0 + br * 1.0;
                float3 sobelY = tl * -1.0 + tm * -2.0 + tr * -1.0 + bl * 1.0 + bm * 2.0 + br * 1.0;
                
                float3 sobel = sqrt(sobelX * sobelX + sobelY * sobelY);
                float edge = dot(sobel, float3(0.299, 0.587, 0.114));
                
                if (edge > edgeThreshold) {
                    return float4(lerp(mm, edgeColor, edgeIntensity), 1.0);
                } else {
                    return float4(mm, 1.0);
                }
            }
        )";
        
        // 셰이더 컴파일 및 생성
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        
        // 버텍스 셰이더
        HRESULT hr = D3DCompile(fullscreenVSSource, strlen(fullscreenVSSource), nullptr, nullptr, nullptr,
                               "main", "vs_4_0", 0, 0, &shaderBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                std::cout << "VS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                errorBlob->Release();
            }
            return false;
        }
        
        hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &fullscreenVS);
        if (FAILED(hr)) {
            shaderBlob->Release();
            return false;
        }
        
        // 입력 레이아웃 생성
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        
        hr = device->CreateInputLayout(layout, 2, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayout);
        shaderBlob->Release();
        if (FAILED(hr)) return false;
        
        // 픽셀 셰이더들 컴파일
        auto compilePS = [&](const char* source, ID3D11PixelShader** shader) -> bool {
            hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr,
                           "main", "ps_4_0", 0, 0, &shaderBlob, &errorBlob);
            if (FAILED(hr)) {
                if (errorBlob) {
                    std::cout << "PS Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                    errorBlob->Release();
                }
                return false;
            }
            
            hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader);
            shaderBlob->Release();
            return SUCCEEDED(hr);
        };
        
        if (!compilePS(brightPassPSSource, &brightPassPS)) return false;
        if (!compilePS(gaussianBlurHPSSource, &gaussianBlurHPS)) return false;
        if (!compilePS(gaussianBlurVPSSource, &gaussianBlurVPS)) return false;
        if (!compilePS(bloomCombinePSSource, &bloomCombinePS)) return false;
        if (!compilePS(motionBlurPSSource, &motionBlurPS)) return false;
        if (!compilePS(edgeDetectionPSSource, &edgeDetectionPS)) return false;
        
        // 상수 버퍼 생성
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(PostProcessParams);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        hr = device->CreateBuffer(&cbDesc, nullptr, &postProcessCB);
        return SUCCEEDED(hr);
    }
    
    bool CreateRenderStates() {
        // 블렌드 상태들
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        HRESULT hr = device->CreateBlendState(&blendDesc, &additiveBlend);
        if (FAILED(hr)) return false;
        
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        
        hr = device->CreateBlendState(&blendDesc, &alphaBlend);
        if (FAILED(hr)) return false;
        
        // 래스터라이저 상태
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.ScissorEnable = false;
        rasterizerDesc.DepthClipEnable = false;
        
        hr = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        if (FAILED(hr)) return false;
        
        // 깊이 스텐실 상태
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = false;
        depthStencilDesc.StencilEnable = false;
        
        hr = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
        if (FAILED(hr)) return false;
        
        // 샘플러 상태들
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        
        hr = device->CreateSamplerState(&samplerDesc, &linearSampler);
        if (FAILED(hr)) return false;
        
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        hr = device->CreateSamplerState(&samplerDesc, &pointSampler);
        return SUCCEEDED(hr);
    }
    
    bool CreateFullscreenQuad() {
        FullscreenVertex vertices[] = {
            { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
        };
        
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        
        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices;
        
        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &fullscreenVB);
        return SUCCEEDED(hr);
    }
    
    void ApplyPostProcessing() {
        if (!device || !context) return;
        
        // 현재 백버퍼를 임시 텍스처로 복사
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        context->CopyResource(tempTexture, backBuffer);
        backBuffer->Release();
        
        // 파라미터 업데이트
        if (paramsChanged.exchange(false)) {
            UpdateParams();
        }
        
        // 효과 적용
        switch (currentEffect) {
            case EffectType::Bloom:
                ApplyBloomEffect();
                break;
            case EffectType::MotionBlur:
                ApplyMotionBlurEffect();
                break;
            case EffectType::EdgeDetection:
                ApplyEdgeDetectionEffect();
                break;
            case EffectType::Combined:
                ApplyCombinedEffects();
                break;
        }
    }
    
    void ApplyBloomEffect() {
        SetupRenderState();
        
        // 1. 브라이트 패스
        context->OMSetRenderTargets(1, &brightRTV, nullptr);
        context->PSSetShader(brightPassPS, nullptr, 0);
        context->PSSetShaderResources(0, 1, &tempSRV);
        DrawFullscreenQuad();
        
        // 2. 가우시안 블러 (다중 패스)
        ID3D11ShaderResourceView* currentSRV = brightSRV;
        
        for (int i = 0; i < BLUR_PASSES; ++i) {
            // 수평 블러
            context->OMSetRenderTargets(1, &blurRTVs[i], nullptr);
            context->PSSetShader(gaussianBlurHPS, nullptr, 0);
            context->PSSetShaderResources(0, 1, &currentSRV);
            DrawFullscreenQuad();
            
            // 수직 블러
            context->OMSetRenderTargets(1, &bloomRTV, nullptr);
            context->PSSetShader(gaussianBlurVPS, nullptr, 0);
            context->PSSetShaderResources(0, 1, &blurSRVs[i]);
            DrawFullscreenQuad();
            
            currentSRV = bloomSRV;
        }
        
        // 3. 블룸 합성
        context->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        context->PSSetShader(bloomCombinePS, nullptr, 0);
        ID3D11ShaderResourceView* srvs[] = { tempSRV, bloomSRV };
        context->PSSetShaderResources(0, 2, srvs);
        DrawFullscreenQuad();
        
        // 셰이더 리소스 해제
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        context->PSSetShaderResources(0, 2, nullSRVs);
    }
    
    void ApplyMotionBlurEffect() {
        SetupRenderState();
        
        // 모션 벡터 계산 (실제로는 더 복잡한 계산 필요)
        // 여기서는 간단한 예시로 이전 프레임과의 차이를 사용
        
        context->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        context->PSSetShader(motionBlurPS, nullptr, 0);
        ID3D11ShaderResourceView* srvs[] = { tempSRV, velocitySRV };
        context->PSSetShaderResources(0, 2, srvs);
        DrawFullscreenQuad();
        
        // 이전 프레임 저장
        context->CopyResource(previousFrameTexture, tempTexture);
        hasPreviousFrame = true;
        
        // 셰이더 리소스 해제
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        context->PSSetShaderResources(0, 2, nullSRVs);
    }
    
    void ApplyEdgeDetectionEffect() {
        SetupRenderState();
        
        context->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        context->PSSetShader(edgeDetectionPS, nullptr, 0);
        context->PSSetShaderResources(0, 1, &tempSRV);
        DrawFullscreenQuad();
        
        // 셰이더 리소스 해제
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
    }
    
    void ApplyCombinedEffects() {
        // 블룸과 엣지 디텍션을 순차적으로 적용
        ApplyBloomEffect();
        
        // 블룸 결과를 임시 텍스처로 복사
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        context->CopyResource(tempTexture, backBuffer);
        backBuffer->Release();
        
        // 엣지 디텍션 적용
        ApplyEdgeDetectionEffect();
    }
    
    void SetupRenderState() {
        // 뷰포트 설정
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(screenWidth);
        viewport.Height = static_cast<float>(screenHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        
        context->RSSetViewports(1, &viewport);
        context->RSSetState(rasterizerState);
        context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        context->OMSetDepthStencilState(depthStencilState, 0);
        
        // 셰이더 설정
        context->VSSetShader(fullscreenVS, nullptr, 0);
        context->IASetInputLayout(inputLayout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        
        // 상수 버퍼 바인딩
        context->PSSetConstantBuffers(0, 1, &postProcessCB);
        
        // 샘플러 바인딩
        context->PSSetSamplers(0, 1, &linearSampler);
        
        // 버텍스 버퍼 설정
        UINT stride = sizeof(FullscreenVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &fullscreenVB, &stride, &offset);
    }
    
    void DrawFullscreenQuad() {
        context->Draw(4, 0);
    }
    
    void UpdateParams() {
        if (!postProcessCB) return;
        
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = context->Map(postProcessCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            memcpy(mappedResource.pData, &params, sizeof(PostProcessParams));
            context->Unmap(postProcessCB, 0);
        }
    }
    
    void StartInputThread() {
        inputThreadRunning = true;
        inputThread = std::thread(&D3D11PostProcessor::InputThreadFunc, this);
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
                CycleEffect();
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F2) & 0x8000) {
                ShowCurrentParams();
                Sleep(200);
            }
            
            // 실시간 파라미터 조정
            const float adjustStep = 0.05f;
            bool changed = false;
            
            if (GetAsyncKeyState('Q') & 0x8000) {
                params.bloomIntensity = max(0.0f, params.bloomIntensity - adjustStep);
                changed = true;
            }
            if (GetAsyncKeyState('W') & 0x8000) {
                params.bloomIntensity = min(2.0f, params.bloomIntensity + adjustStep);
                changed = true;
            }
            
            if (GetAsyncKeyState('A') & 0x8000) {
                params.bloomThreshold = max(0.0f, params.bloomThreshold - adjustStep);
                changed = true;
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                params.bloomThreshold = min(1.0f, params.bloomThreshold + adjustStep);
                changed = true;
            }
            
            if (GetAsyncKeyState('Z') & 0x8000) {
                params.motionBlurStrength = max(0.0f, params.motionBlurStrength - adjustStep);
                changed = true;
            }
            if (GetAsyncKeyState('X') & 0x8000) {
                params.motionBlurStrength = min(1.0f, params.motionBlurStrength + adjustStep);
                changed = true;
            }
            
            if (changed) {
                paramsChanged = true;
            }
            
            Sleep(50);
        }
    }
    
    void CycleEffect() {
        int effectIndex = static_cast<int>(currentEffect);
        effectIndex = (effectIndex + 1) % 5;
        currentEffect = static_cast<EffectType>(effectIndex);
        
        const wchar_t* effectNames[] = { L"None", L"Bloom", L"Motion Blur", L"Edge Detection", L"Combined" };
        std::wcout << L"효과 변경: " << effectNames[effectIndex] << std::endl;
    }
    
    void ShowControls() {
        std::wcout << L"\n=== 포스트 프로세싱 컨트롤 ===" << std::endl;
        std::wcout << L"F1: 효과 변경" << std::endl;
        std::wcout << L"F2: 현재 설정 보기" << std::endl;
        std::wcout << L"Q/W: 블룸 강도 조정" << std::endl;
        std::wcout << L"A/S: 블룸 임계값 조정" << std::endl;
        std::wcout << L"Z/X: 모션 블러 강도 조정" << std::endl;
        std::wcout << L"===============================\n" << std::endl;
    }
    
    void ShowCurrentParams() {
        const wchar_t* effectNames[] = { L"None", L"Bloom", L"Motion Blur", L"Edge Detection", L"Combined" };
        
        std::wcout << L"\n=== 현재 설정 ===" << std::endl;
        std::wcout << L"효과: " << effectNames[static_cast<int>(currentEffect)] << std::endl;
        std::wcout << L"블룸 강도: " << params.bloomIntensity << std::endl;
        std::wcout << L"블룸 임계값: " << params.bloomThreshold << std::endl;
        std::wcout << L"모션 블러 강도: " << params.motionBlurStrength << std::endl;
        std::wcout << L"엣지 임계값: " << params.edgeThreshold << std::endl;
        std::wcout << L"================\n" << std::endl;
    }
    
    void Cleanup() {
        StopInputThread();
        
        // 셰이더 정리
        if (fullscreenVS) { fullscreenVS->Release(); fullscreenVS = nullptr; }
        if (brightPassPS) { brightPassPS->Release(); brightPassPS = nullptr; }
        if (gaussianBlurHPS) { gaussianBlurHPS->Release(); gaussianBlurHPS = nullptr; }
        if (gaussianBlurVPS) { gaussianBlurVPS->Release(); gaussianBlurVPS = nullptr; }
        if (bloomCombinePS) { bloomCombinePS->Release(); bloomCombinePS = nullptr; }
        if (motionBlurPS) { motionBlurPS->Release(); motionBlurPS = nullptr; }
        if (edgeDetectionPS) { edgeDetectionPS->Release(); edgeDetectionPS = nullptr; }
        
        // 버퍼와 상태 정리
        if (fullscreenVB) { fullscreenVB->Release(); fullscreenVB = nullptr; }
        if (postProcessCB) { postProcessCB->Release(); postProcessCB = nullptr; }
        if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
        if (linearSampler) { linearSampler->Release(); linearSampler = nullptr; }
        if (pointSampler) { pointSampler->Release(); pointSampler = nullptr; }
        if (additiveBlend) { additiveBlend->Release(); additiveBlend = nullptr; }
        if (alphaBlend) { alphaBlend->Release(); alphaBlend = nullptr; }
        if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
        if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }
        
        CleanupRenderTargets();
        
        if (context) { context->Release(); context = nullptr; }
        if (device) { device->Release(); device = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    }
};

// 정적 멤버 정의
D3D11PostProcessor* D3D11PostProcessor::instance = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11PostProcessor::OriginalPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11PostProcessor::OriginalResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT) = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static D3D11PostProcessor* processor = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"D3D11 포스트 프로세서 DLL 로드됨" << std::endl;
            
            processor = new D3D11PostProcessor();
            if (!processor->InstallHook()) {
                delete processor;
                processor = nullptr;
                std::wcout << L"포스트 프로세서 설치 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (processor) {
                processor->UninstallHook();
                delete processor;
                processor = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}