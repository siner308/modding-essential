# Visual Effects Example Code

DirectX 11 게임에 실시간 시각 효과를 주입하는 완전한 C++ 구현 예제입니다.

## 📁 파일 구조

```
example-code/
├── D3D11Hook.h            # DirectX 11 후킹 시스템 헤더
├── D3D11Hook.cpp          # DirectX 후킹 구현
├── VisualEffects.cpp      # 시각 효과 및 셰이더 시스템
├── main.cpp               # 메인 애플리케이션 및 UI
├── CMakeLists.txt         # CMake 빌드 스크립트
└── README.md              # 이 파일
```

## 🚀 빌드 방법

### 필수 요구사항

1. **Microsoft Detours** - 함수 후킹 라이브러리
2. **DirectX 11 SDK** - Windows SDK에 포함
3. **Visual Studio 2019 이상** - MSVC 컴파일러

### Windows (Visual Studio)

```bash
# 1. Microsoft Detours 설치
git clone https://github.com/Microsoft/Detours.git
cd Detours
nmake
# Detours를 시스템 경로에 설치하거나 CMakeLists.txt에서 경로 지정

# 2. 프로젝트 빌드
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

# 또는 제공된 배치 파일 사용
build.bat
```

### 의존성 설치

```bash
# vcpkg를 사용한 의존성 관리 (권장)
vcpkg install detours:x64-windows
vcpkg integrate install

# CMake에서 vcpkg 사용
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

## 💻 사용법

### 기본 사용

```cpp
#include "D3D11Hook.h"

int main() {
    // 1. DirectX 11 후킹 시스템 초기화
    if (!D3D11Hook::Initialize()) {
        std::cout << "초기화 실패" << std::endl;
        return -1;
    }
    
    // 2. 시각 효과 시스템이 자동으로 활성화됨
    std::cout << "시각 효과가 적용되었습니다!" << std::endl;
    
    // 3. 애플리케이션 실행 (게임이 DirectX를 사용하는 동안)
    // 메뉴 시스템을 통해 실시간으로 효과 조정 가능
    
    // 4. 종료 시 정리
    D3D11Hook::Shutdown();
    return 0;
}
```

### 프리셋 사용

```cpp
// 미리 정의된 효과 프리셋 로드
VisualEffects::LoadPreset("cinematic");    // 영화같은 효과
VisualEffects::LoadPreset("vintage");      // 빈티지 효과
VisualEffects::LoadPreset("cyberpunk");    // 사이버펑크 효과
```

### 수동 파라미터 조정

```cpp
EffectParams params = VisualEffects::GetEffectParams();

// 기본 색상 조정
params.brightness = 1.2f;     // 밝기 증가
params.contrast = 1.3f;       // 대비 증가
params.saturation = 1.1f;     // 채도 증가
params.gamma = 1.0f;          // 감마 보정

// 색상 틴트 (RGB 배율)
params.colorTint = {1.1f, 1.0f, 0.9f}; // 따뜻한 톤

// 특수 효과
params.enableSepia = 1.0f;           // 세피아 효과 활성화
params.vignetteStrength = 0.3f;      // 비네팅 강도
params.sharpenStrength = 0.2f;       // 샤프닝 강도

// 고급 색상 보정
params.shadows = {0.95f, 0.98f, 1.0f};    // 그림자 (푸른 톤)
params.midtones = {1.0f, 1.0f, 1.0f};     // 중간톤 (중성)
params.highlights = {1.0f, 1.0f, 0.98f};  // 하이라이트 (따뜻한 톤)

VisualEffects::SetEffectParams(params);
```

## 🎮 지원 게임

### 테스트된 게임들

- ✅ **Elden Ring** - 완전 지원 (모든 효과 작동)
- ✅ **Dark Souls III** - 완전 지원
- ✅ **The Witcher 3** - 완전 지원
- ✅ **Skyrim SE** - 완전 지원
- ⚠️ **온라인 게임** - 안티치트 때문에 사용 불가

### 게임별 권장 설정

```cpp
// 게임에 따른 최적화된 프리셋
if (gameTitle == "eldenring.exe") {
    VisualEffects::LoadPreset("cinematic");  // 영화같은 분위기
} else if (gameTitle == "witcher3.exe") {
    VisualEffects::LoadPreset("natural");    // 자연스러운 강화
} else if (gameTitle == "skyrim.exe") {
    VisualEffects::LoadPreset("warm");       // 따뜻한 판타지 톤
}
```

## 🔧 핵심 기능

### 1. DirectX 11 후킹

```cpp
// Present() 함수 후킹으로 매 프레임마다 효과 적용
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    // 시각 효과 적용
    if (VisualEffects::IsEnabled()) {
        VisualEffects::ApplyEffects();
    }
    
    // 원본 Present 호출
    return oPresent(pSwapChain, SyncInterval, Flags);
}
```

### 2. 실시간 셰이더 컴파일

```cpp
// 런타임에 셰이더 수정 및 재컴파일
const char* customShaderSource = R"(
    float4 main(VS_OUTPUT input) : SV_Target {
        float3 color = MainTexture.Sample(MainSampler, input.uv).rgb;
        
        // 커스텀 효과 적용
        color = ApplyColorGrading(color);
        color = ApplySpecialEffects(color, input.uv);
        
        return float4(color, 1.0);
    }
)";

ID3D11PixelShader* shader = ShaderManager::CompilePixelShader(customShaderSource);
```

### 3. 다중 패스 렌더링

```cpp
// 복잡한 효과를 위한 다중 패스 처리
void VisualEffects::ApplyEffects() {
    // Pass 1: 색상 보정
    ApplyColorGrading();
    
    // Pass 2: 특수 효과 (블룸, 샤프닝 등)
    ApplySpecialEffects();
    
    // Pass 3: 최종 톤 매핑
    ApplyToneMapping();
}
```

## 🎨 사용 가능한 효과

### 기본 색상 조정
- **밝기 (Brightness)**: -1.0 ~ 3.0
- **대비 (Contrast)**: 0.0 ~ 3.0  
- **채도 (Saturation)**: 0.0 ~ 3.0
- **감마 (Gamma)**: 0.1 ~ 3.0
- **색상 틴트 (Color Tint)**: RGB 배율

### 고급 색상 보정
- **삼색 보정**: 그림자/중간톤/하이라이트 개별 조정
- **색상 균형**: 자동 화이트 밸런스
- **선택적 색상**: 특정 색상 범위만 조정

### 특수 효과
- **세피아 톤**: 빈티지 갈색 효과
- **흑백**: 모노크롬 변환
- **색상 반전**: 네거티브 효과
- **비네팅**: 가장자리 어둡게
- **샤프닝**: 이미지 선명도 증가
- **필름 그레인**: 노이즈 추가로 아날로그 느낌

### 프로페셔널 프리셋
- **Cinematic**: 영화적 분위기 (따뜻한 톤, 비네팅)
- **Vintage**: 빈티지 필름 (세피아, 그레ين)
- **High Contrast**: 선명하고 생생한 색감
- **Dramatic**: 강한 명암대비
- **Cyberpunk**: 네온 색감, 높은 채도

## ⚠️ 주의사항

### 시스템 요구사항

1. **관리자 권한 필수**: DirectX 후킹을 위해 필요
2. **Windows 10/11**: DirectX 11 지원 OS
3. **DirectX 11 게임**: DirectX 9/12 게임은 미지원
4. **Microsoft Detours**: 필수 의존성 라이브러리

### 안전 가이드라인

```cpp
// 안티치트 감지 방지를 위한 주의사항
1. 오프라인 게임에서만 사용
2. Steam Overlay, Discord Overlay 비활성화
3. 녹화 소프트웨어 종료
4. 방화벽에서 게임 네트워크 차단
```

### 호환성 문제

**Q: 게임이 시작되지 않아요**
```
A: 다음을 확인하세요:
1. 안티바이러스 실시간 보호 비활성화
2. Windows Defender 예외 추가
3. 관리자 권한으로 실행
4. DirectX 11 지원 게임인지 확인
```

**Q: 효과가 적용되지 않아요**
```
A: 다음을 시도하세요:
1. 게임이 전체화면 모드인지 확인
2. VSync 설정 확인
3. 다른 오버레이 소프트웨어 종료
4. DirectX 11 모드로 게임 실행
```

**Q: 성능이 크게 떨어져요**
```
A: 최적화 방법:
1. 효과 강도 낮추기
2. 복잡한 효과 (블룸, 샤프닝) 비활성화
3. 해상도 낮추기
4. 멀티패스 효과 제한
```

## 📊 성능 모니터링

### 실시간 FPS 측정

```cpp
// 성능 프로파일러 사용
EffectProfiler::BeginFrame();
// ... 렌더링 코드 ...
EffectProfiler::EndFrame();

// 통계 확인
float avgFrameTime = EffectProfiler::GetAverageFrameTime();
float currentFPS = EffectProfiler::GetCurrentFPS();

std::cout << "평균 프레임 시간: " << avgFrameTime << "ms" << std::endl;
std::cout << "현재 FPS: " << currentFPS << std::endl;
```

### 효과별 성능 영향

| 효과 | 성능 영향 | 권장 설정 |
|------|-----------|-----------|
| 기본 색상 조정 | 매우 낮음 | 모든 게임 |
| 비네팅 | 낮음 | 모든 게임 |
| 샤프닝 | 중간 | 고사양 PC |
| 블룸 | 높음 | 최고사양 PC |
| 멀티패스 효과 | 매우 높음 | 개발/테스트용 |

## 🔗 관련 자료

- [DirectX 11 후킹 가이드](../../getting-started/directx-hooking-guide.md)
- [셰이더 프로그래밍 기초](../../getting-started/shader-programming-guide.md)
- [시각 효과 수정](../README.md)
- [Microsoft Detours 문서](https://github.com/Microsoft/Detours)

## 🚀 고급 사용법

### 커스텀 셰이더 작성

```hlsl
// 커스텀 포스트 프로세싱 셰이더
float4 CustomEffect(VS_OUTPUT input) : SV_Target {
    float3 color = MainTexture.Sample(MainSampler, input.uv).rgb;
    
    // 여기에 커스텀 효과 로직 추가
    color = YourCustomFunction(color);
    
    return float4(color, 1.0);
}
```

### 핫 리로드 시스템

```cpp
// 개발 중 셰이더 파일 변경 시 자동 재컴파일
ShaderManager::EnableHotReload(true);

// 메인 루프에서 주기적으로 확인
while (running) {
    ShaderManager::CheckForUpdates();
    // ... 게임 루프 ...
}
```

---

**⚡ 빌드 후 `build/bin/Release/VisualEffects.exe`를 관리자 권한으로 실행하세요!**

**🎮 DirectX 11 게임을 실행한 후 이 프로그램을 시작하면 실시간으로 시각 효과가 적용됩니다.**