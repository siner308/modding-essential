## 1. 게임 상태 시스템 이해 (Understanding the Game State System)

### 개요 (Overview)

게임 상태 시스템(Game State System) 또는 유한 상태 머신(Finite State Machine, FSM)은 게임의 현재 상황을 정의하고 관리하는 핵심적인 프로그래밍 패턴입니다. 플레이어가 메뉴를 보는지, 게임을 플레이하는지, 로딩 중인지 등을 명확한 '상태(state)'로 구분하여, 각 상황에 맞는 로직만 실행되도록 제어합니다.

모딩, 특히 게임의 흐름을 제어하는 모드(예: 일시정지)를 만들 때 이 시스템을 이해하는 것은 매우 중요합니다.

### 주요 게임 상태 (Common Game States)

대부분의 게임은 다음과 같은 기본 상태들을 가집니다.

- **`GAME_RUNNING`** (또는 `PLAYING`): 플레이어가 실제로 게임을 조작하고 있는 정상적인 상태입니다. 물리, AI, 렌더링 등 모든 시스템이 활성화됩니다.
- **`GAME_PAUSED`**: 게임이 일시정지된 상태입니다. 게임 로직(AI, 물리 등)은 멈추지만, 메뉴 UI 등은 계속 업데이트될 수 있습니다.
- **`GAME_MENU`** (또는 `MAIN_MENU`, `IN_GAME_MENU`): 메인 메뉴나 설정 창 같은 UI와 상호작용하는 상태입니다.
- **`GAME_LOADING`**: 레벨이나 데이터를 불러오는 중인 상태입니다. 로딩 화면이나 팁이 표시됩니다.
- **`CUTSCENE`**: 컷신이 재생되는 상태로, 플레이어의 입력이 제한됩니다.

### 메모리에서의 표현 (Representation in Memory)

게임 내부에서 현재 상태는 주로 정수(Integer)나 열거형(Enum) 변수로 저장됩니다.

예를 들어, `GAME_RUNNING`은 `0`, `GAME_PAUSED`는 `1`, `GAME_MENU`는 `2`와 같이 숫자와 매핑됩니다. 우리의 목표는 메모리 스캔을 통해 이 '상태 변수'가 저장된 메모리 주소를 찾아내는 것입니다.

### 게임 루프에서의 활용 예시 (Example in a Game Loop)

게임 엔진은 매 프레임마다 '게임 루프(Game Loop)'를 실행하며 현재 상태를 확인하고 그에 맞는 동작을 수행합니다.

```cpp
// 게임 엔진의 메인 루프 (가상 코드)
void MainGameLoop() {
    while (true) {
        // 현재 게임 상태를 읽어옴
        GameState currentState = GetCurrentGameState();

        switch (currentState) {
            case GAME_RUNNING:
                ProcessPlayerInput();
                UpdateAI();
                UpdatePhysics();
                RenderGameWorld();
                break;

            case GAME_PAUSED:
                ProcessMenuInput(); // 일시정지 메뉴 처리
                RenderPauseMenu();
                break;

            case GAME_MENU:
                ProcessMenuInput();
                RenderMainMenu();
                break;

            case GAME_LOADING:
                RenderLoadingScreen();
                break;
        }
    }
}
```

### 일시정지 모딩의 핵심 아이디어 (Core Idea for Pause Modding)

온라인 게임 등 일시정지 기능이 없는 게임은 `GAME_PAUSED` 상태가 없거나, 있더라도 플레이어가 직접 진입할 수 없는 경우가 많습니다.

우리가 하려는 것은 이 게임 루프의 흐름을 강제로 바꾸는 것입니다.

예를 들어, `currentState`가 항상 `GAME_RUNNING`(`0`)일 때, 특정 조건(핫키 입력)이 만족되면 `UpdateAI`, `UpdatePhysics` 같은 핵심 로직을 건너뛰도록 어셈블리 코드를 수정하는 것입니다. 이는 `cmp` (비교) 명령어의 결과를 조작하거나, `call` (함수 호출) 명령어를 `nop` (아무것도 안 함)으로 덮어쓰는 방식으로 이루어집니다.

---

**다음 단계**: [메모리 스캔 기법](../getting-started/memory-scanning-guide.md)
