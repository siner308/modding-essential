/*
 * Exercise 2: 간단한 후킹
 * 
 * 문제: D3D11의 Present() 함수를 후킹하여 화면에 "Hello World"를 출력하세요.
 * 
 * 학습 목표:
 * - DirectX 11 후킹 기초
 * - Present() 함수 가로채기
 * - 텍스트 렌더링 구현
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

class D3D11SimpleHook {
private:
    static D3D11SimpleHook* instance;
    
    // D3D11 리소스
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;
    
    // 텍스트 렌더링을 위한 리소스
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    ID3D11Buffer* constantBuffer = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11BlendState* blendState = nullptr;
    ID3D11RasterizerState* rasterizerState = nullptr;
    ID3D11Texture2D* fontTexture = nullptr;
    ID3D11ShaderResourceView* fontSRV = nullptr;
    ID3D11SamplerState* fontSampler = nullptr;
    
    // 후킹 관련
    static HRESULT (STDMETHODCALLTYPE* OriginalPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT (STDMETHODCALLTYPE* OriginalResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    
    // 상태 관리
    bool initialized = false;
    bool hookInstalled = false;
    UINT screenWidth = 0;
    UINT screenHeight = 0;
    
    // 텍스트 렌더링 구조체
    struct TextVertex {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
        XMFLOAT4 color;
    };
    
    struct ConstantBufferData {
        XMMATRIX transform;
        XMFLOAT4 textColor;
    };

public:
    D3D11SimpleHook() {
        instance = this;
    }
    
    ~D3D11SimpleHook() {
        Cleanup();
        instance = nullptr;
    }
    
    bool InstallHook() {
        if (hookInstalled) {
            return true;
        }
        
        std::wcout << L"D3D11 후킹 시작..." << std::endl;
        
        // 임시 D3D11 디바이스 생성하여 VTable 주소 획득
        if (!CreateTempDevice()) {
            std::wcout << L"임시 디바이스 생성 실패" << std::endl;
            return false;
        }
        
        // Present 함수 후킹
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OriginalPresent, HookedPresent);
        DetourAttach(&(PVOID&)OriginalResizeBuffers, HookedResizeBuffers);
        
        if (DetourTransactionCommit() == NO_ERROR) {
            hookInstalled = true;
            std::wcout << L"D3D11 후킹 성공" << std::endl;
            return true;
        } else {
            std::wcout << L"D3D11 후킹 실패" << std::endl;
            return false;
        }
    }
    
    void UninstallHook() {
        if (!hookInstalled) {
            return;
        }
        
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)OriginalPresent, HookedPresent);
        DetourDetach(&(PVOID&)OriginalResizeBuffers, HookedResizeBuffers);
        DetourTransactionCommit();
        
        hookInstalled = false;
        std::wcout << L"D3D11 후킹 해제됨" << std::endl;
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
            // VTable에서 함수 주소 추출
            void** swapChainVTable = *reinterpret_cast<void***>(tempSwapChain);
            OriginalPresent = reinterpret_cast<decltype(OriginalPresent)>(swapChainVTable[8]); // Present
            OriginalResizeBuffers = reinterpret_cast<decltype(OriginalResizeBuffers)>(swapChainVTable[13]); // ResizeBuffers
            
            // 임시 리소스 해제
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
                std::wcout << L"D3D11 리소스 초기화 완료" << std::endl;
            } else {
                return;
            }
        }
        
        // "Hello World" 텍스트 렌더링
        RenderText();
    }
    
    void OnResizeBuffers() {
        CleanupRenderTarget();
        initialized = false;
    }
    
    bool InitializeResources(IDXGISwapChain* pSwapChain) {
        // SwapChain에서 디바이스와 컨텍스트 획득
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));
        if (FAILED(hr)) return false;
        
        device->GetImmediateContext(&context);
        swapChain = pSwapChain;
        swapChain->AddRef();
        
        // 백버퍼와 렌더 타겟 뷰 생성
        if (!CreateRenderTarget()) return false;
        
        // 화면 크기 획득
        DXGI_SWAP_CHAIN_DESC desc;
        swapChain->GetDesc(&desc);
        screenWidth = desc.BufferDesc.Width;
        screenHeight = desc.BufferDesc.Height;
        
        // 셰이더 생성
        if (!CreateShaders()) return false;
        
        // 폰트 텍스처 생성
        if (!CreateFontTexture()) return false;
        
        // 렌더링 상태 생성
        if (!CreateRenderStates()) return false;
        
        // 텍스트 지오메트리 생성
        if (!CreateTextGeometry()) return false;
        
        return true;
    }
    
    bool CreateRenderTarget() {
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        if (FAILED(hr)) return false;
        
        hr = device->CreateRenderTargetView(backBuffer, nullptr, &mainRenderTargetView);
        backBuffer->Release();
        
        return SUCCEEDED(hr);
    }
    
    void CleanupRenderTarget() {
        if (mainRenderTargetView) {
            mainRenderTargetView->Release();
            mainRenderTargetView = nullptr;
        }
    }
    
    bool CreateShaders() {
        // 간단한 2D 텍스트 렌더링용 셰이더
        const char* vertexShaderSource = R"(
            cbuffer ConstantBuffer : register(b0) {
                matrix transform;
                float4 textColor;
            };
            
            struct VS_INPUT {
                float3 pos : POSITION;
                float2 tex : TEXCOORD0;
                float4 color : COLOR;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
                float4 color : COLOR;
            };
            
            PS_INPUT main(VS_INPUT input) {
                PS_INPUT output;
                output.pos = mul(float4(input.pos, 1.0f), transform);
                output.tex = input.tex;
                output.color = input.color;
                return output;
            }
        )";
        
        const char* pixelShaderSource = R"(
            Texture2D fontTexture : register(t0);
            SamplerState fontSampler : register(s0);
            
            cbuffer ConstantBuffer : register(b0) {
                matrix transform;
                float4 textColor;
            };
            
            struct PS_INPUT {
                float4 pos : SV_POSITION;
                float2 tex : TEXCOORD0;
                float4 color : COLOR;
            };
            
            float4 main(PS_INPUT input) : SV_Target {
                float alpha = fontTexture.Sample(fontSampler, input.tex).r;
                return float4(textColor.rgb, textColor.a * alpha);
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
        
        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
        if (FAILED(hr)) {
            vsBlob->Release();
            return false;
        }
        
        // 입력 레이아웃 생성
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        
        hr = device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
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
        
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
        psBlob->Release();
        
        if (FAILED(hr)) return false;
        
        // 상수 버퍼 생성
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(ConstantBufferData);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
        return SUCCEEDED(hr);
    }
    
    bool CreateFontTexture() {
        // 간단한 8x8 비트맵 폰트 생성 (ASCII 32-127)
        const int FONT_WIDTH = 8;
        const int FONT_HEIGHT = 8;
        const int CHARS_PER_ROW = 16;
        const int CHAR_ROWS = 6; // 96글자 / 16 = 6행
        const int TEXTURE_WIDTH = FONT_WIDTH * CHARS_PER_ROW;
        const int TEXTURE_HEIGHT = FONT_HEIGHT * CHAR_ROWS;
        
        // 간단한 비트맵 폰트 데이터 (Hello World에 필요한 글자들만)
        std::vector<BYTE> fontData(TEXTURE_WIDTH * TEXTURE_HEIGHT, 0);
        
        // 'H' (ASCII 72, 0x48)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'H' - 32, {
            0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00
        });
        
        // 'e' (ASCII 101, 0x65)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'e' - 32, {
            0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3E, 0x00
        });
        
        // 'l' (ASCII 108, 0x6C)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'l' - 32, {
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x1E, 0x00
        });
        
        // 'o' (ASCII 111, 0x6F)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'o' - 32, {
            0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00
        });
        
        // ' ' (공백, ASCII 32, 0x20)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, ' ' - 32, {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        });
        
        // 'W' (ASCII 87, 0x57)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'W' - 32, {
            0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00
        });
        
        // 'r' (ASCII 114, 0x72)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'r' - 32, {
            0x00, 0x00, 0x6E, 0x70, 0x60, 0x60, 0x60, 0x00
        });
        
        // 'd' (ASCII 100, 0x64)
        CreateCharBitmap(fontData, TEXTURE_WIDTH, 'd' - 32, {
            0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00
        });
        
        // 텍스처 생성
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = TEXTURE_WIDTH;
        textureDesc.Height = TEXTURE_HEIGHT;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = fontData.data();
        initData.SysMemPitch = TEXTURE_WIDTH;
        
        HRESULT hr = device->CreateTexture2D(&textureDesc, &initData, &fontTexture);
        if (FAILED(hr)) return false;
        
        // 셰이더 리소스 뷰 생성
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        
        hr = device->CreateShaderResourceView(fontTexture, &srvDesc, &fontSRV);
        if (FAILED(hr)) return false;
        
        // 샘플러 생성
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        
        hr = device->CreateSamplerState(&samplerDesc, &fontSampler);
        return SUCCEEDED(hr);
    }
    
    void CreateCharBitmap(std::vector<BYTE>& fontData, int textureWidth, int charIndex, const std::vector<BYTE>& pattern) {
        const int FONT_WIDTH = 8;
        const int FONT_HEIGHT = 8;
        const int CHARS_PER_ROW = 16;
        
        int charX = (charIndex % CHARS_PER_ROW) * FONT_WIDTH;
        int charY = (charIndex / CHARS_PER_ROW) * FONT_HEIGHT;
        
        for (int y = 0; y < FONT_HEIGHT; ++y) {
            BYTE row = pattern[y];
            for (int x = 0; x < FONT_WIDTH; ++x) {
                bool pixel = (row & (0x80 >> x)) != 0;
                int pixelIndex = (charY + y) * textureWidth + (charX + x);
                fontData[pixelIndex] = pixel ? 255 : 0;
            }
        }
    }
    
    bool CreateRenderStates() {
        // 블렌드 상태 생성 (알파 블렌딩)
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        HRESULT hr = device->CreateBlendState(&blendDesc, &blendState);
        if (FAILED(hr)) return false;
        
        // 래스터라이저 상태 생성
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.ScissorEnable = false;
        rasterizerDesc.DepthClipEnable = false;
        
        hr = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        return SUCCEEDED(hr);
    }
    
    bool CreateTextGeometry() {
        // "Hello World" 텍스트를 위한 쿼드 생성
        std::string text = "Hello World";
        std::vector<TextVertex> vertices;
        std::vector<UINT> indices;
        
        const float charWidth = 20.0f;
        const float charHeight = 32.0f;
        const float startX = 50.0f;
        const float startY = 50.0f;
        
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            float x = startX + i * charWidth;
            float y = startY;
            
            // 문자의 UV 좌표 계산
            int charIndex = c - 32;
            int charCol = charIndex % 16;
            int charRow = charIndex / 6;
            
            float u1 = charCol / 16.0f;
            float v1 = charRow / 6.0f;
            float u2 = (charCol + 1) / 16.0f;
            float v2 = (charRow + 1) / 6.0f;
            
            // 쿼드 정점 추가
            UINT baseIndex = static_cast<UINT>(vertices.size());
            
            vertices.push_back({XMFLOAT3(x, y, 0.0f), XMFLOAT2(u1, v1), XMFLOAT4(1, 1, 1, 1)});
            vertices.push_back({XMFLOAT3(x + charWidth, y, 0.0f), XMFLOAT2(u2, v1), XMFLOAT4(1, 1, 1, 1)});
            vertices.push_back({XMFLOAT3(x + charWidth, y + charHeight, 0.0f), XMFLOAT2(u2, v2), XMFLOAT4(1, 1, 1, 1)});
            vertices.push_back({XMFLOAT3(x, y + charHeight, 0.0f), XMFLOAT2(u1, v2), XMFLOAT4(1, 1, 1, 1)});
            
            // 인덱스 추가
            indices.insert(indices.end(), {
                baseIndex, baseIndex + 1, baseIndex + 2,
                baseIndex, baseIndex + 2, baseIndex + 3
            });
        }
        
        // 버텍스 버퍼 생성
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(TextVertex));
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        
        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices.data();
        
        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
        if (FAILED(hr)) return false;
        
        // 인덱스 버퍼 생성
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(UINT));
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        
        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indices.data();
        
        hr = device->CreateBuffer(&ibDesc, &ibData, &indexBuffer);
        return SUCCEEDED(hr);
    }
    
    void RenderText() {
        if (!device || !context || !mainRenderTargetView) return;
        
        // 뷰포트 설정
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(screenWidth);
        viewport.Height = static_cast<float>(screenHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        
        // 이전 상태 백업
        ID3D11RenderTargetView* oldRTV = nullptr;
        ID3D11DepthStencilView* oldDSV = nullptr;
        context->OMGetRenderTargets(1, &oldRTV, &oldDSV);
        
        // 직교 투영 행렬 생성
        XMMATRIX orthoMatrix = XMMatrixOrthographicOffCenterLH(
            0.0f, static_cast<float>(screenWidth),
            static_cast<float>(screenHeight), 0.0f,
            0.0f, 1.0f
        );
        
        // 상수 버퍼 업데이트
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            ConstantBufferData* cbData = reinterpret_cast<ConstantBufferData*>(mappedResource.pData);
            cbData->transform = XMMatrixTranspose(orthoMatrix);
            cbData->textColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
            context->Unmap(constantBuffer, 0);
        }
        
        // 렌더링 상태 설정
        context->RSSetViewports(1, &viewport);
        context->RSSetState(rasterizerState);
        
        float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        context->OMSetBlendState(blendState, blendFactor, 0xffffffff);
        
        // 셰이더 설정
        context->VSSetShader(vertexShader, nullptr, 0);
        context->PSSetShader(pixelShader, nullptr, 0);
        context->IASetInputLayout(inputLayout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        // 상수 버퍼 바인딩
        context->VSSetConstantBuffers(0, 1, &constantBuffer);
        context->PSSetConstantBuffers(0, 1, &constantBuffer);
        
        // 텍스처와 샘플러 바인딩
        context->PSSetShaderResources(0, 1, &fontSRV);
        context->PSSetSamplers(0, 1, &fontSampler);
        
        // 버텍스 버퍼 설정
        UINT stride = sizeof(TextVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        
        // 그리기
        context->DrawIndexed(66, 0, 0); // "Hello World" = 11글자 * 6 인덱스 = 66
        
        // 이전 상태 복원
        context->OMSetRenderTargets(1, &oldRTV, oldDSV);
        if (oldRTV) oldRTV->Release();
        if (oldDSV) oldDSV->Release();
    }
    
    void Cleanup() {
        if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
        if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
        if (constantBuffer) { constantBuffer->Release(); constantBuffer = nullptr; }
        if (vertexShader) { vertexShader->Release(); vertexShader = nullptr; }
        if (pixelShader) { pixelShader->Release(); pixelShader = nullptr; }
        if (inputLayout) { inputLayout->Release(); inputLayout = nullptr; }
        if (blendState) { blendState->Release(); blendState = nullptr; }
        if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
        if (fontTexture) { fontTexture->Release(); fontTexture = nullptr; }
        if (fontSRV) { fontSRV->Release(); fontSRV = nullptr; }
        if (fontSampler) { fontSampler->Release(); fontSampler = nullptr; }
        
        CleanupRenderTarget();
        
        if (context) { context->Release(); context = nullptr; }
        if (device) { device->Release(); device = nullptr; }
        if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    }
};

// 정적 멤버 정의
D3D11SimpleHook* D3D11SimpleHook::instance = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11SimpleHook::OriginalPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
HRESULT (STDMETHODCALLTYPE* D3D11SimpleHook::OriginalResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT) = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static D3D11SimpleHook* hook = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"D3D11 Simple Hook DLL 로드됨" << std::endl;
            
            hook = new D3D11SimpleHook();
            if (!hook->InstallHook()) {
                delete hook;
                hook = nullptr;
                std::wcout << L"후킹 설치 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (hook) {
                hook->UninstallHook();
                delete hook;
                hook = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}

// 독립 실행형 인젝터 (선택사항)
#ifdef STANDALONE_INJECTOR
int main() {
    std::wcout << L"=== D3D11 Simple Hook 인젝터 ===" << std::endl;
    std::wcout << L"대상 프로세스 이름을 입력하세요: ";
    
    std::wstring processName;
    std::wcin >> processName;
    
    // 프로세스 찾기 및 DLL 인젝션 코드
    // (실제 구현은 별도의 인젝션 라이브러리 필요)
    
    return 0;
}
#endif