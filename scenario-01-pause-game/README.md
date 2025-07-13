# 🟢 Scenario 01: 게임 일시정지 구현

**난이도**: 초급 | **학습 시간**: 1-2주 | **접근법**: 바이너리 모딩 (본격 시작)

온라인 게임에 실시간 일시정지 기능을 추가하는 모딩 기법을 학습합니다.



## 📖 학습 목표

이 시나리오를 완료하면 다음을 할 수 있게 됩니다:

- [ ] 게임 상태 변수를 메모리에서 찾기
- [ ] 어셈블리 코드에서 조건부 점프 이해하기
- [ ] JE(Jump if Equal)와 JNE(Jump if Not Equal) 변환하기
- [ ] 안전한 메모리 패치 적용하기
- [ ] DLL 인젝션을 통한 모드 구현하기

## 🎯 최종 결과물

완성된 모드의 기능:
- **P키** 또는 **컨트롤러 조합**으로 게임 일시정지/재개
- **설정 파일**을 통한 키바인드 커스터마이징
- **안전한 패치** 적용 및 복원
- **실시간 상태 표시** (로그를 통한 피드백)

## 📚 학습 흐름

```
1. 이론 학습 (30분)
   ├── 게임 상태 시스템 이해
   ├── 조건부 점프 원리
   └── 메모리 패치 개념

2. 도구 실습 (1시간)
   ├── Cheat Engine으로 메모리 스캔
   ├── 게임 상태 변수 찾기
   └── 어셈블리 코드 분석

3. 코드 분석 (1시간)
   ├── EldenRing PauseTheGame 모드 분석
   ├── AOB 패턴 이해
   └── 패치 로직 파악

4. 실습 구현 (2-3시간)
   ├── 기본 DLL 프로젝트 생성
   ├── 메모리 스캔 코드 작성
   ├── 패치 적용 코드 구현
   └── 키 입력 처리 추가

5. 응용 연습 (1시간)
   ├── 다른 게임에 적용해보기
   ├── 추가 기능 구현
   └── 오류 처리 개선
```

## 🔧 필요한 도구

### 필수 도구
- ✅ [Cheat Engine](https://cheatengine.org/) - 메모리 분석
- ✅ [Visual Studio](https://visualstudio.microsoft.com/) - 코드 개발
- ✅ [x64dbg](https://x64dbg.com/) - 어셈블리 분석 (선택사항)

### 테스트 게임
- 🎮 **EldenRing** (주 예제) - 오프라인 모드
- 🎮 **Solitaire** (초급 연습용) - Windows 기본 게임
- 🎮 **Minesweeper** (중급 연습용) - Windows 기본 게임

### 배경 지식
- 📚 **C++ 기초** (포인터, 기본 문법)
- 📚 **어셈블리 기초** (MOV, CMP, JE/JNE)
- 📚 **Windows API** (기본적인 이해)

## 🔍 핵심 개념 미리보기

### 1. 게임 상태 시스템
```cpp
// 대부분의 게임에서 사용하는 패턴
enum GameState {
    GAME_RUNNING = 0,    // 정상 진행
    GAME_PAUSED = 1,     // 일시정지
    GAME_MENU = 2,       // 메뉴 상태
    GAME_LOADING = 3     // 로딩 중
};

// 게임 루프에서의 활용
if (current_state == GAME_RUNNING) {
    UpdateGameLogic();   // 게임 로직 실행
    UpdateAI();
    UpdatePhysics();
} else {
    UpdateUIOnly();      // UI만 업데이트
}
```

### 2. 어셈블리 조건부 점프
```assembly
; 원본: 정상적인 일시정지 로직
cmp byte ptr [game_state], 0    ; 게임 상태가 0(진행중)인가?
je skip_game_logic              ; 0이 아니면(일시정지) 게임 로직 건너뛰기
call UpdateGameLogic            ; 게임 로직 실행
skip_game_logic:

; 패치 후: 역전된 로직
cmp byte ptr [game_state], 0    ; 게임 상태가 0(진행중)인가?
jne skip_game_logic             ; 0이면(진행중) 게임 로직 건너뛰기!
call UpdateGameLogic            ; 이제 게임이 항상 멈춤
skip_game_logic:
```

### 3. AOB (Array of Bytes) 패턴
```
원본 어셈블리:
  0f 84 12 34 56 78    ; JE (상대 주소 포함)
  c6 45 fc 00          ; MOV byte ptr [rbp-4], 0

AOB 패턴 (와일드카드 사용):
  0f 84 ? ? ? ?        ; JE (주소는 게임마다 다르므로 ?)
  c6 ? ? 00            ; MOV (오프셋 다양하므로 ?)
```

## 📋 각 단계별 상세 내용

### [1. 게임 상태 시스템 이해](./01-understanding-game-state.md)
- 게임 엔진의 상태 관리 방식
- 일시정지 구현의 다양한 접근법
- 메모리에서 상태 변수 위치 추론

### [2. 메모리 스캔 기법](../getting-started/memory-scanning-guide.md)
- Cheat Engine을 이용한 값 찾기
- 동적 메모리 주소 추적
- 포인터 체인 분석

### [3. 어셈블리 코드 분석](../reference/assembly-quick-reference.md)
- x86-64 조건부 점프 명령어
- 플래그 레지스터와 비교 연산
- 게임별 어셈블리 패턴 분석

### [4. 메모리 패치 구현](../getting-started/memory-patching-guide.md)
- 안전한 패치 적용 방법
- 백업 및 복원 메커니즘
- 에러 처리 및 검증

### [5. DLL 인젝션 완성](./example-code/DllMain.cpp)
- DLL 프로젝트 설정
- 프로세스 인젝션 기법
- 키 입력 처리 시스템

### [6. TypeScript 구현 버전](./typescript-implementation.md) 🆕
- TypeScript/Node.js로 같은 모드 구현
- C++과의 차이점 및 장단점 비교
- FFI를 통한 Windows API 호출
- 개발 환경 설정 및 실행 방법

## 🎮 실제 예제 분석

### EldenRing PauseTheGame 모드
```cpp
// 핵심 패치 코드 (example-code/analysis.md에서 상세 분석)
bool TogglePause() {
    static bool isPaused = false;
    
    if (!isPaused) {
        // JE(0x84) → JNE(0x85) 패치
        ReplaceExpectedBytesAtAddress(patchAddress + 1, "84", "85");
        isPaused = true;
        Log("Game Paused");
    } else {
        // JNE(0x85) → JE(0x84) 복원
        ReplaceExpectedBytesAtAddress(patchAddress + 1, "85", "84");
        isPaused = false;
        Log("Game Resumed");
    }
    
    return isPaused;
}
```

### 구현 방법 비교
```
C++ (DLL) 버전:              TypeScript (Node.js) 버전:
├── Visual Studio 필요       ├── Node.js만 설치 
├── Windows API 직접 호출    ├── FFI를 통한 API 호출
├── 컴파일 필요 (.dll)       ├── 인터프리터 실행 (.js)
├── 빠른 실행 속도          ├── 빠른 개발 속도
└── 게임과 함께 로드        └── 별도 프로세스 실행
```

### 다른 게임 예제들
- **Skyrim**: 시간 배율 조정을 통한 일시정지
- **Dark Souls**: 유사한 FromSoftware 엔진 패턴
- **Generic Unity Game**: 일반적인 Time.timeScale 조작

## 🚀 심화 학습 방향

### 이 시나리오 완료 후 도전할 수 있는 것들:

#### **즉시 도전 가능**
- 다른 FromSoftware 게임에 적용 (Dark Souls 시리즈)
- TypeScript 버전으로 GUI 개발 (Electron 활용)
- 키바인드 시스템 고도화 (설정 파일 확장)
- 일시정지 상태 시각화 (화면 오버레이)

#### **추가 학습 후 도전**
- [Scenario 03: FromSoftware 게임 모딩](../scenario-03-fromsoftware-modding/) - 전문 도구 활용
- [Scenario 04: PAK 모드 로더](../scenario-04-pak-loader/) - 모듈화 시스템
- 범용 모드 로더 시스템 구축 (C++ + TypeScript 하이브리드)

## 📊 진도 체크

### 체크포인트 1: 이론 이해 (1일차)
- [ ] 게임 상태 시스템 개념 이해
- [ ] 조건부 점프 명령어 구분 가능
- [ ] AOB 패턴의 의미 파악

### 체크포인트 2: 도구 활용 (2일차)
- [ ] Cheat Engine으로 값 찾기 성공
- [ ] 메모리 주소에 접근하는 코드 발견
- [ ] 어셈블리 뷰에서 조건부 점프 확인

### 체크포인트 3: 코드 분석 (3일차)  
- [ ] 예제 모드의 AOB 패턴 이해
- [ ] 패치 로직의 동작 원리 파악
- [ ] 각 함수의 역할 설명 가능

### 체크포인트 4: 실습 완성 (4-5일차)
- [ ] 기본 DLL 프로젝트 컴파일 성공
- [ ] 메모리 스캔 및 패치 적용 성공
- [ ] 키 입력으로 일시정지 토글 가능

### 체크포인트 5: 응용 및 최적화 (6-7일차)
- [ ] 다른 게임에 적용 시도
- [ ] 에러 처리 및 안전성 개선
- [ ] 추가 기능 구현 (설정, 로깅 등)

## 🆘 자주 묻는 질문

### Q: 패치가 적용되지 않아요
```
A: 다음을 확인해보세요:
1. 게임 버전이 예제와 동일한가?
2. AOB 패턴이 올바르게 매칭되는가?
3. 관리자 권한으로 실행했는가?
4. 안티치트/안티바이러스가 차단하지 않나?
```

### Q: 게임이 크래시됩니다
```
A: 안전 조치:
1. 즉시 게임 종료
2. 백업에서 원본 파일 복원
3. 패치 코드의 주소 및 크기 재확인
4. 더 작은 단위로 테스트 진행
```

### Q: 다른 게임에도 적용할 수 있나요?
```
A: 가능하지만 주의사항:
1. 게임 엔진에 따라 패턴 상이
2. 각 게임의 이용약관 확인 필요
3. 온라인 기능이 있다면 오프라인에서만 테스트
4. 항상 백업 후 진행
```

---

**🎯 준비되셨나요?**

[1. 게임 상태 시스템 이해](./01-understanding-game-state.md)부터 시작하여 첫 번째 게임 모드를 만들어보세요!

---

**다음 학습**: [Scenario 02: FPS 제한 해제](../scenario-02-unlock-fps/)

**⚡ 완료 예상 시간**: 5-7일 (하루 1-2시간 투자 기준)