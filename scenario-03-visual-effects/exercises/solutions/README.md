# Exercise Solutions - 시각 효과 수정

이 폴더는 scenario-03-visual-effects의 연습문제 해답들을 포함합니다.

## 📋 연습문제 목록

### Exercise 1: DirectX 버전 감지
**문제**: 게임이 사용하는 DirectX 버전(9/11/12)을 자동으로 감지하는 프로그램을 작성하세요.

**해답 파일**: `exercise1_dx_detection.cpp`

### Exercise 2: 간단한 후킹
**문제**: D3D11의 Present() 함수를 후킹하여 화면에 "Hello World"를 출력하세요.

**해답 파일**: `exercise2_simple_hook.cpp`

### Exercise 3: 색상 필터
**문제**: 실시간으로 화면의 색상을 조정하는 필터를 구현하세요 (밝기, 대비, 채도).

**해답 파일**: `exercise3_color_filter.cpp`

### Exercise 4: 셰이더 교체
**문제**: 게임의 특정 셰이더를 커스텀 셰이더로 교체하는 시스템을 만드세요.

**해답 파일**: `exercise4_shader_replacement.cpp`

### Exercise 5: 포스트 프로세싱 효과
**문제**: 블룸, 엣지 디텍션, 모션 블러 중 하나를 구현하세요.

**해답 파일**: `exercise5_postprocess.cpp`

## 🎨 학습 목표

### 그래픽스 프로그래밍
1. **DirectX API**: D3D11 기본 사용법
2. **셰이더 프로그래밍**: HLSL 기초
3. **렌더링 파이프라인**: GPU 처리 과정 이해
4. **텍스처 조작**: 실시간 이미지 처리

### 시스템 프로그래밍
1. **DLL 인젝션**: 프로세스 메모리 접근
2. **함수 후킹**: API 가로채기
3. **메모리 관리**: GPU 메모리 다루기
4. **멀티스레딩**: 렌더링 스레드 동기화

## 🛠️ 기술 스택

### 필수 라이브러리
```cpp
// DirectX 11
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// Windows API
#include <Windows.h>
#include <detours.h>  // Microsoft Detours
```

### 셰이더 예제
```hlsl
// 기본 포스트 프로세싱 셰이더
Texture2D MainTexture : register(t0);
SamplerState MainSampler : register(s0);

cbuffer EffectParams : register(b0) {
    float brightness;
    float contrast;
    float saturation;
    float gamma;
};

float4 main(VS_OUTPUT input) : SV_Target {
    float3 color = MainTexture.Sample(MainSampler, input.uv).rgb;
    
    // 밝기 조정
    color *= brightness;
    
    // 대비 조정
    color = ((color - 0.5) * contrast) + 0.5;
    
    // 채도 조정
    float gray = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(gray.xxx, color, saturation);
    
    // 감마 보정
    color = pow(abs(color), gamma);
    
    return float4(color, 1.0);
}
```

## 🎮 대상 게임

### 테스트 게임
- **Elden Ring**: DirectX 12
- **Dark Souls III**: DirectX 11
- **Skyrim SE**: DirectX 11
- **The Witcher 3**: DirectX 11

### 엔진별 특성
```cpp
// Unreal Engine 4 후킹 포인트
namespace UE4 {
    const char* PRESENT_PATTERN = "48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC 20";
    const char* DRAWCALL_PATTERN = "40 53 48 83 EC 20 44 8B 41 04";
}

// Unity 엔진 후킹 포인트
namespace Unity {
    const char* PRESENT_PATTERN = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18";
    const char* CAMERA_PATTERN = "48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9";
}
```

## 📊 성능 최적화

### GPU 성능 고려사항
```cpp
// 효율적인 셰이더 컴파일
ID3DBlob* CompileShaderOptimized(const char* source, const char* entryPoint, const char* profile) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr,
                           entryPoint, profile, flags, 0, &shaderBlob, &errorBlob);
    
    if (FAILED(hr) && errorBlob) {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        errorBlob->Release();
    }
    
    return shaderBlob;
}
```

### 메모리 관리
```cpp
// RAII 기반 리소스 관리
template<typename T>
class D3DResource {
    T* resource;
public:
    D3DResource(T* res) : resource(res) {}
    ~D3DResource() { if (resource) resource->Release(); }
    T* Get() { return resource; }
    T** GetAddressOf() { return &resource; }
};
```

## ⚠️ 주의사항

### 호환성 문제
1. **DirectX 버전**: 게임의 DX 버전 확인
2. **안티치트**: EAC, BattlEye 등 우회 불가
3. **드라이버**: 그래픽 드라이버 호환성
4. **성능**: FPS 드롭 최소화

### 디버깅 팁
```cpp
// D3D11 디버그 레이어 활성화
#ifdef _DEBUG
UINT creationFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
UINT creationFlags = 0;
#endif

D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags,
                  nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
```

## 🔍 검증 방법

### 기능 테스트
1. **시각적 확인**: 효과가 제대로 적용되는가?
2. **성능 측정**: FPS 저하가 최소한인가?
3. **안정성**: 장시간 실행 시 메모리 누수 없는가?
4. **호환성**: 다른 모드와 충돌하지 않는가?

### 자동화 테스트
```cpp
// 스크린샷 기반 검증
bool VerifyEffect(ID3D11DeviceContext* context, ID3D11Texture2D* renderTarget) {
    // 렌더 타겟을 CPU로 복사
    D3D11_MAPPED_SUBRESOURCE mapped;
    context->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    
    // 픽셀 데이터 분석
    DWORD* pixels = (DWORD*)mapped.pData;
    bool effectApplied = AnalyzePixelData(pixels, width, height);
    
    context->Unmap(stagingTexture, 0);
    return effectApplied;
}
```

---

**🎨 목표: 실시간 그래픽스 조작을 통해 게임의 시각적 경험을 향상시키는 기술 습득**