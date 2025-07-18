/*
 * Exercise 3: 색상 필터
 * 
 * 문제: 실시간으로 화면의 색상을 조정하는 필터를 구현하세요 (밝기, 대비, 채도).
 * 
 * 학습 목표:
 * - 포스트 프로세싱 셰이더 구현
 * - 실시간 색상 조정
 * - 사용자 인터페이스 연동
 */

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <detours.h>
#include <iostream>
#include <string>
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

class D3D11ColorFilter {
private:
    static D3D11ColorFilter* instance;
    
    // D3D11 리소스
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;
    
    // 포스트 프로세싱 리소스
    ID3D11Texture2D* postProcessTexture = nullptr;
    ID3D11RenderTargetView* postProcessRTV = nullptr;
    ID3D11ShaderResourceView* postProcessSRV = nullptr;
    ID3D11Buffer* postProcessVB = nullptr;
    ID3D11Buffer* postProcessCB = nullptr;
    ID3D11VertexShader* postProcessVS = nullptr;
    ID3D11PixelShader* postProcessPS = nullptr;
    ID3D11InputLayout* postProcessLayout = nullptr;
    ID3D11SamplerState* postProcessSampler = nullptr;
    ID3D11BlendState* blendState = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;
    ID3D11DepthStencilState* depthStencilState = nullptr;
    
    // 후킹 관련
    static HRESULT (STDMETHODCALLTYPE* OriginalPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT (STDMETHODCALLTYPE* OriginalResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    
    // 상태 관리
    bool initialized = false;
    bool hookInstalled = false;
    bool filterEnabled = true;
    UINT screenWidth = 0;
    UINT screenHeight = 0;
    
    // 색상 필터 파라미터
    struct ColorFilterParams {
        float brightness = 1.0f;    // 0.0 ~ 2.0
        float contrast = 1.0f;      // 0.0 ~ 2.0
        float saturation = 1.0f;    // 0.0 ~ 2.0
        float gamma = 1.0f;         // 0.5 ~ 2.5
        float hue = 0.0f;           // -180 ~ 180 degrees
        float temperature = 0.0f;   // -100 ~ 100 (color temperature)
        float vibrance = 0.0f;      // -1.0 ~ 1.0
        float exposure = 0.0f;      // -2.0 ~ 2.0
    };
    
    ColorFilterParams filterParams;
    std::atomic<bool> paramsChanged{false};
    
    // 프리셋 시스템
    struct FilterPreset {
        std::string name;
        ColorFilterParams params;
    };
    
    std::vector<FilterPreset> presets;
    int currentPreset = 0;
    
    // 입력 제어 스레드
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning{false};
    
    // 렌더링 구조체
    struct PostProcessVertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
    };

public:
    D3D11ColorFilter() {
        instance = this;
        InitializePresets();
    }
    
    ~D3D11ColorFilter() {
        Cleanup();
        instance = nullptr;
    }
    
    bool InstallHook() {
        if (hookInstalled) {
            return true;
        }
        
        std::wcout << L"D3D11 색상 필터 후킹 시작..." << std::endl;
        
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
            std::wcout << L"색상 필터 후킹 성공" << std::endl;
            ShowControls();
            return true;
        } else {
            std::wcout << L"색상 필터 후킹 실패" << std::endl;
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
        std::wcout << L"색상 필터 후킹 해제됨" << std::endl;
    }

private:
    void InitializePresets() {
        // 기본 프리셋들
        presets.push_back({"Default", {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}});
        presets.push_back({"Bright", {1.3f, 1.1f, 1.1f, 1.0f, 0.0f, 0.0f, 0.2f, 0.3f}});
        presets.push_back({"Dark", {0.7f, 1.2f, 0.9f, 1.1f, 0.0f, 0.0f, -0.1f, -0.2f}});
        presets.push_back({"Vivid", {1.1f, 1.3f, 1.4f, 1.0f, 0.0f, 0.0f, 0.4f, 0.2f}});
        presets.push_back({"Warm", {1.0f, 1.0f, 1.0f, 1.0f, 10.0f, 20.0f, 0.1f, 0.0f}});
        presets.push_back({"Cool", {1.0f, 1.0f, 1.0f, 1.0f, -10.0f, -20.0f, 0.1f, 0.0f}});
        presets.push_back({"Cinematic", {0.9f, 1.4f, 0.8f, 0.9f, 0.0f, 15.0f, -0.2f, -0.1f}});
        presets.push_back({"High Contrast", {1.0f, 1.8f, 1.2f, 1.0f, 0.0f, 0.0f, 0.3f, 0.0f}});
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
                std::wcout << L"색상 필터 리소스 초기화 완료" << std::endl;
            } else {
                return;
            }
        }
        
        if (filterEnabled) {
            ApplyColorFilter();
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
        
        // 포스트 프로세싱 셰이더 생성
        if (!CreatePostProcessShaders()) return false;
        
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
        
        // 포스트 프로세싱용 임시 텍스처
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = screenWidth;
        textureDesc.Height = screenHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        
        hr = device->CreateTexture2D(&textureDesc, nullptr, &postProcessTexture);
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(postProcessTexture, nullptr, &postProcessRTV);
        if (FAILED(hr)) return false;
        
        hr = device->CreateShaderResourceView(postProcessTexture, nullptr, &postProcessSRV);
        if (FAILED(hr)) return false;
        
        return true;
    }
    
    void CleanupRenderTargets() {
        if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
        if (postProcessTexture) { postProcessTexture->Release(); postProcessTexture = nullptr; }
        if (postProcessRTV) { postProcessRTV->Release(); postProcessRTV = nullptr; }
        if (postProcessSRV) { postProcessSRV->Release(); postProcessSRV = nullptr; }
    }
    
    bool CreatePostProcessShaders() {
        // 버텍스 셰이더
        const char* vertexShaderSource = R"(
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
        
        // 고급 색상 필터 픽셀 셰이더
        const char* pixelShaderSource = R"(
            Texture2D MainTexture : register(t0);
            SamplerState MainSampler : register(s0);
            
            cbuffer ColorFilterParams : register(b0) {
                float brightness;
                float contrast;
                float saturation;
                float gamma;
                float hue;
                float temperature;
                float vibrance;
                float exposure;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
            };
            
            // RGB를 HSV로 변환
            float3 rgb2hsv(float3 c) {
                float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
                float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
                float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
                
                float d = q.x - min(q.w, q.y);
                float e = 1.0e-10;
                return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
            }
            
            // HSV를 RGB로 변환
            float3 hsv2rgb(float3 c) {
                float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
            }
            
            // 색온도 조정
            float3 adjustTemperature(float3 color, float temp) {
                float t = temp / 100.0;
                float3 kelvin = float3(1.0, 1.0, 1.0);
                
                if (t > 0) {
                    // 따뜻하게 (노란색/주황색)
                    kelvin.r = 1.0 + t * 0.2;
                    kelvin.g = 1.0 + t * 0.1;
                    kelvin.b = 1.0 - t * 0.3;
                } else {
                    // 차갑게 (파란색)
                    kelvin.r = 1.0 + t * 0.3;
                    kelvin.g = 1.0 + t * 0.1;
                    kelvin.b = 1.0 - t * 0.2;
                }
                
                return color * kelvin;
            }
            
            // 비브런스 조정 (채도와 다름 - 이미 채도가 높은 부분은 덜 영향)
            float3 adjustVibrance(float3 color, float vibrance) {
                float gray = dot(color, float3(0.299, 0.587, 0.114));
                float mask = clamp(1.0 - abs(gray - 0.5) * 2.0, 0.0, 1.0);
                return lerp(color, lerp(gray.xxx, color, 1.0 + vibrance), mask);
            }
            
            float4 main(PS_INPUT input) : SV_Target {
                float3 color = MainTexture.Sample(MainSampler, input.tex).rgb;
                
                // 노출 조정 (톤매핑 전)
                color *= pow(2.0, exposure);
                
                // 밝기 조정
                color *= brightness;
                
                // 대비 조정
                color = ((color - 0.5) * contrast) + 0.5;
                
                // 감마 보정
                color = pow(abs(color), gamma);
                
                // 색온도 조정
                color = adjustTemperature(color, temperature);
                
                // 색상(Hue) 조정
                if (abs(hue) > 0.001) {
                    float3 hsv = rgb2hsv(color);
                    hsv.x = frac(hsv.x + hue / 360.0);
                    color = hsv2rgb(hsv);
                }
                
                // 채도 조정
                if (abs(saturation - 1.0) > 0.001) {
                    float gray = dot(color, float3(0.299, 0.587, 0.114));
                    color = lerp(gray.xxx, color, saturation);
                }
                
                // 비브런스 조정
                if (abs(vibrance) > 0.001) {
                    color = adjustVibrance(color, vibrance);
                }
                
                // 최종 클램핑
                color = saturate(color);
                
                return float4(color, 1.0);
            }
        )";
        
        // 버텍스 셰이더 컴파일
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        
        HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
                               "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
        
        if (FAILED(hr)) {
            if (errorBlob) {
                std::wcerr << L"VS Compile Error: " << StringToWString(reinterpret_cast<char*>(errorBlob->GetBufferPointer())) << std::endl;
                errorBlob->Release();
            }
            return false;
        }
        
        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &postProcessVS);
        if (FAILED(hr)) {
            vsBlob->Release();
            return false;
        }
        
        // 입력 레이아웃 생성
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        
        hr = device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &postProcessLayout);
        vsBlob->Release();
        if (FAILED(hr)) return false;
        
        // 픽셀 셰이더 컴파일
        ID3DBlob* psBlob = nullptr;
        hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
                       "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
        
        if (FAILED(hr)) {
            if (errorBlob) {
                std::wcerr << L"PS Compile Error: " << StringToWString(reinterpret_cast<char*>(errorBlob->GetBufferPointer())) << std::endl;
                errorBlob->Release();
            }
            return false;
        }
        
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &postProcessPS);
        psBlob->Release();
        if (FAILED(hr)) return false;
        
        // 상수 버퍼 생성
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(ColorFilterParams);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        hr = device->CreateBuffer(&cbDesc, nullptr, &postProcessCB);
        return SUCCEEDED(hr);
    }
    
    bool CreateRenderStates() {
        // 블렌드 상태
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = false;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        HRESULT hr = device->CreateBlendState(&blendDesc, &blendState);
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
        
        // 샘플러 상태
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        
        hr = device->CreateSamplerState(&samplerDesc, &postProcessSampler);
        return SUCCEEDED(hr);
    }
    
    bool CreateFullscreenQuad() {
        PostProcessVertex vertices[] = {
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
        
        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &postProcessVB);
        return SUCCEEDED(hr);
    }
    
    void ApplyColorFilter() {
        if (!device || !context || !mainRenderTargetView || !postProcessRTV) return;
        
        // 현재 백버퍼를 임시 텍스처로 복사
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        context->CopyResource(postProcessTexture, backBuffer);
        backBuffer->Release();
        
        // 색상 필터 파라미터 업데이트
        if (paramsChanged.exchange(false)) {
            UpdateFilterParams();
        }
        
        // 뷰포트 설정
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(screenWidth);
        viewport.Height = static_cast<float>(screenHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        
        // 렌더링 상태 설정
        context->RSSetViewports(1, &viewport);
        context->RSSetState(rasterizerState);
        context->OMSetBlendState(blendState, nullptr, 0xffffffff);
        context->OMSetDepthStencilState(depthStencilState, 0);
        context->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        
        // 셰이더 설정
        context->VSSetShader(postProcessVS, nullptr, 0);
        context->PSSetShader(postProcessPS, nullptr, 0);
        context->IASetInputLayout(postProcessLayout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        
        // 상수 버퍼 바인딩
        context->PSSetConstantBuffers(0, 1, &postProcessCB);
        
        // 텍스처와 샘플러 바인딩
        context->PSSetShaderResources(0, 1, &postProcessSRV);
        context->PSSetSamplers(0, 1, &postProcessSampler);
        
        // 버텍스 버퍼 설정
        UINT stride = sizeof(PostProcessVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &postProcessVB, &stride, &offset);
        
        // 풀스크린 쿼드 그리기
        context->Draw(4, 0);
        
        // 셰이더 리소스 해제
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
    }
    
    void UpdateFilterParams() {
        if (!postProcessCB) return;
        
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = context->Map(postProcessCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            memcpy(mappedResource.pData, &filterParams, sizeof(ColorFilterParams));
            context->Unmap(postProcessCB, 0);
        }
    }
    
    void StartInputThread() {
        inputThreadRunning = true;
        inputThread = std::thread(&D3D11ColorFilter::InputThreadFunc, this);
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
                filterEnabled = !filterEnabled;
                std::wcout << L"색상 필터: " << (filterEnabled ? L"켜짐" : L"꺼짐") << std::endl;
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F2) & 0x8000) {
                currentPreset = (currentPreset + 1) % presets.size();
                filterParams = presets[currentPreset].params;
                paramsChanged = true;
                std::wcout << L"프리셋 변경: " << StringToWString(presets[currentPreset].name) << std::endl;
                Sleep(200);
            }
            
            if (GetAsyncKeyState(VK_F3) & 0x8000) {
                ShowCurrentParams();
                Sleep(200);
            }
            
            // 실시간 조정 (키보드)
            const float adjustStep = 0.05f;
            
            if (GetAsyncKeyState('Q') & 0x8000) {
                filterParams.brightness = max(0.1f, filterParams.brightness - adjustStep);
                paramsChanged = true;
            }
            if (GetAsyncKeyState('W') & 0x8000) {
                filterParams.brightness = min(3.0f, filterParams.brightness + adjustStep);
                paramsChanged = true;
            }
            
            if (GetAsyncKeyState('A') & 0x8000) {
                filterParams.contrast = max(0.1f, filterParams.contrast - adjustStep);
                paramsChanged = true;
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                filterParams.contrast = min(3.0f, filterParams.contrast + adjustStep);
                paramsChanged = true;
            }
            
            if (GetAsyncKeyState('Z') & 0x8000) {
                filterParams.saturation = max(0.0f, filterParams.saturation - adjustStep);
                paramsChanged = true;
            }
            if (GetAsyncKeyState('X') & 0x8000) {
                filterParams.saturation = min(3.0f, filterParams.saturation + adjustStep);
                paramsChanged = true;
            }
            
            if (GetAsyncKeyState('E') & 0x8000) {
                filterParams.gamma = max(0.1f, filterParams.gamma - adjustStep);
                paramsChanged = true;
            }
            if (GetAsyncKeyState('R') & 0x8000) {
                filterParams.gamma = min(3.0f, filterParams.gamma + adjustStep);
                paramsChanged = true;
            }
            
            Sleep(50); // 50ms 간격으로 체크
        }
    }
    
    void ShowControls() {
        std::wcout << L"\n=== 색상 필터 컨트롤 ===" << std::endl;
        std::wcout << L"F1: 필터 켜기/끄기" << std::endl;
        std::wcout << L"F2: 프리셋 변경" << std::endl;
        std::wcout << L"F3: 현재 설정 보기" << std::endl;
        std::wcout << L"Q/W: 밝기 조정" << std::endl;
        std::wcout << L"A/S: 대비 조정" << std::endl;
        std::wcout << L"Z/X: 채도 조정" << std::endl;
        std::wcout << L"E/R: 감마 조정" << std::endl;
        std::wcout << L"===========================\n" << std::endl;
    }
    
    void ShowCurrentParams() {
        std::wcout << L"\n=== 현재 설정 ===" << std::endl;
        std::wcout << L"프리셋: " << StringToWString(presets[currentPreset].name) << std::endl;
        std::wcout << L"밝기: " << filterParams.brightness << std::endl;
        std::wcout << L"대비: " << filterParams.contrast << std::endl;
        std::wcout << L"채도: " << filterParams.saturation << std::endl;
        std::wcout << L"감마: " << filterParams.gamma << std::endl;
        std::wcout << L"색조: " << filterParams.hue << std::endl;
        std::wcout << L"색온도: " << filterParams.temperature << std::endl;
        std::wcout << L"비브런스: " << filterParams.vibrance << std::endl;
        std::wcout << L"노출: " << filterParams.exposure << std::endl;
        std::wcout << L"================\n" << std::endl;
    }
    
    void Cleanup() {
        StopInputThread();
        
        if (postProcessVB) { postProcessVB->Release(); postProcessVB = nullptr; }
        if (postProcessCB) { postProcessCB->Release(); postProcessCB = nullptr; }
        if (postProcessVS) { postProcessVS->Release(); postProcessVS = nullptr; }
        if (postProcessPS) { postProcessPS->Release(); postProcessPS = nullptr; }
        if (postProcessLayout) { postProcessLayout->Release(); postProcessLayout = nullptr; }
        if (postProcessSampler) { postProcessSampler->Release(); postProcessSampler = nullptr; }
        if (blendState) { blendState->Release(); blendState = nullptr; }
        if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
        if (depthStencilState) { depthStencilState->Release(); depthStencilState = nullptr; }
        
        CleanupRenderTargets();
        
        if (context) { context->Release(); context = nullptr; }
        if (device) { device->Release(); device = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    }
};

// 정적 멤버 정의
D3D11ColorFilter* D3D11ColorFilter::instance = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11ColorFilter::OriginalPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11ColorFilter::OriginalResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT) = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static D3D11ColorFilter* filter = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"D3D11 색상 필터 DLL 로드됨" << std::endl;
            
            filter = new D3D11ColorFilter();
            if (!filter->InstallHook()) {
                delete filter;
                filter = nullptr;
                std::wcout << L"색상 필터 설치 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (filter) {
                filter->UninstallHook();
                delete filter;
                filter = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}