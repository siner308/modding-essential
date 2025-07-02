# PAK 모딩 예제 코드

이 디렉터리에는 언리얼 엔진 게임의 기본적인 PAK 모딩 기술을 보여주는 예제 코드가 포함되어 있습니다.

## PakModExample.cpp

이 예제는 간단한 PAK 모드가 게임과 어떻게 상호작용하는지 보여줍니다. 실제 시나리오에서는 이 코드가 DLL로 컴파일된 후 `.pak` 파일로 패키징되어 게임 엔진에 의해 로드됩니다.

### 주요 개념:

- **콘솔 명령어**: 모드가 게임 내 콘솔 명령어를 등록하고 실행하는 방법.
- **게임 상태 상호작용**: 게임 상태(예: 무적 모드 토글, 게임 속도 변경)와의 기본적인 상호작용.
- **PAK 파일 구조**: 코드에 직접적으로 표시되지는 않지만, 이 모드는 `.pak` 파일 내에 패키징되어 게임 엔진의 PAK 로딩 우선순위 시스템을 활용하도록 설계되었습니다.

### 사용법 (개념적):

1.  `PakModExample.cpp`를 DLL로 컴파일합니다.
2.  DLL (및 기타 에셋)을 `.pak` 파일(예: `MyMod_P.pak`)로 패키징합니다.
3.  `.pak` 파일을 게임의 `Content/Paks/~mods/` 디렉터리에 배치합니다.
4.  게임을 실행하고 등록된 콘솔 명령어를 사용합니다.

```cpp
// 간단한 콘솔 명령어 등록 예제
// (실제 구현은 엔진별 API를 포함합니다)
void RegisterConsoleCommand(const char* commandName, void (*callback)()) {
    // ... 엔진별 등록 로직 ...
}

// 콘솔 명령어 함수 예제
void ToggleGodMode() {
    // ... 무적 모드를 활성화/비활성화하기 위한 게임별 메모리 쓰기 ...
    // 예: WriteMemory(PlayerHealthAddress, 99999);
}

// 모드 진입점
void OnModLoad() {
    RegisterConsoleCommand("toggle_god", ToggleGodMode);
    // ... 기타 초기화 ...
}

void OnModUnload() {
    // ... 정리 ...
}
```
