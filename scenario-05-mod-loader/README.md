# 🟡 Scenario 05: 모드 로더 구현

**난이도**: 중급-고급 | **학습 시간**: 2-3주 | **접근법**: DLL 인젝션 + API 디자인

게임에 모드를 동적으로 로드하고 관리하는 모드 로더 시스템을 직접 구현합니다.

## 📖 학습 목표

이 시나리오를 완료하면 다음을 할 수 있게 됩니다:

- [ ] DLL 동적 로딩 및 언로딩 메커니즘 이해하기
- [ ] 모드 간 통신을 위한 API 인터페이스 설계하기
- [ ] 모드 설정 저장 및 로드 시스템 구현하기
- [ ] 모드 의존성 분석 및 해결하기
- [ ] 핫 리로드(Hot Reload) 기능 구현하여 개발 효율 높이기

## 🎯 최종 결과물

완성된 모드 로더의 기능:
- **자동 모드 검색 및 로드** (지정된 폴더에서 DLL 모드)
- **모드 활성화/비활성화** 및 **재로드** 기능
- **모드 간 통신 채널** (이벤트 시스템, 공유 인터페이스)
- **모드별 설정 파일** 관리
- **의존성 자동 해결** 및 **순환 참조 탐지**
- **개발 중 핫 리로드** 지원

## 📚 모드 로더 아키텍처

### 1. DLL 동적 로딩
```cpp
// LoadLibrary와 GetProcAddress를 사용하여 DLL 로드 및 함수 포인터 획득
HMODULE hMod = LoadLibrary(L"MyMod.dll");
if (hMod) {
    typedef IGameMod* (*CreateModFunc)();
    CreateModFunc createMod = (CreateModFunc)GetProcAddress(hMod, "CreateMod");
    if (createMod) {
        IGameMod* modInstance = createMod();
        // ... 모드 사용
    }
}
```

### 2. 모드 API 디자인
```cpp
// 모드들이 사용할 수 있는 공통 인터페이스
class IModAPI {
public:
    virtual ILogger* GetLogger() = 0;
    virtual IConfigManager* GetConfig() = 0;
    virtual void FireEvent(const Event& event) = 0;
    // ... 기타 기능
};

// 모드 인터페이스
class IGameMod {
public:
    virtual bool Initialize(IModAPI* api) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    // ... 모드 정보
};
```

### 3. 의존성 해결
```
모드 A -> 모드 B (A는 B에 의존)
모드 로더는 B를 먼저 로드해야 함

그래프 이론: 위상 정렬 (Topological Sort)
- 노드: 각 모드
- 엣지: 의존성 (방향성)
- 순환 참조: 사이클 탐지
```

### 4. 핫 리로드
```
개발 중 코드 변경 시 게임 재시작 없이 모드 업데이트

원리:
1. 파일 시스템 변경 감지 (ReadDirectoryChangesW)
2. 기존 DLL 언로드 (FreeLibrary)
3. 새 DLL 로드 (LoadLibrary)
4. 모드 인스턴스 재초기화
```

## 💻 구현 단계별 가이드

### [1. 기본 DLL 로더 구현](./exercises/solutions/exercise1_basic_loader.cpp)
- DLL 로딩 및 언로딩
- 모드 정보 추출
- 간단한 모드 목록 관리

### [2. 모드 API 시스템 설계 및 구현](./exercises/solutions/exercise2_mod_api.cpp)
- IModAPI 인터페이스 정의
- 로깅, 설정, 이벤트 시스템 구현
- 모드 간 인터페이스 공유

### [3. 설정 관리 시스템 구현](./exercises/solutions/exercise3_config_system.cpp)
- INI 파일 파싱 및 저장
- 타입 안전한 설정 접근
- 설정 변경 콜백

### [4. 의존성 해결 시스템 구현](./exercises/solutions/exercise4_dependency_resolver.cpp)
- 모드 메타데이터 파싱
- 의존성 그래프 구축
- 위상 정렬 알고리즘 적용
- 순환 참조 및 충돌 탐지

### [5. 핫 리로드 시스템 구현](./exercises/solutions/exercise5_hot_reload.cpp)
- 파일 시스템 변경 감지 스레드
- 디바운싱 및 재시도 로직
- 백업 및 복원 기능
- 모드 재로드 콜백 연동

## 🚀 심화 학습 방향

### 이 시나리오 완료 후 도전할 수 있는 것들:
- **GUI 기반 모드 로더**: Qt, Electron, WPF 등을 활용
- **모드 스토어 통합**: 온라인 모드 저장소와 연동
- **플러그인 시스템**: 런타임에 모드 추가/제거
- **샌드박싱**: 모드 실행 환경 격리 (안전성 강화)
- **스크립팅 엔진 통합**: Lua, Python 등으로 모드 개발 지원

## 🔗 관련 자료

- [DLL Injection Techniques](https://www.ired.team/offensive-security/code-injection-process-injection/dll-injection) - DLL 인젝션 기술
- [Detours Library](https://github.com/microsoft/detours) - Detours 라이브러리
- [Topological Sort Algorithm](https://en.wikipedia.org/wiki/Topological_sorting) - 위상 정렬 알고리즘
- [Hot Reloading in C++](https://www.gamasutra.com/view/news/179983/Indie_Game_Dev_Tutorial_Hot_reloading_your_C_code.php) - C++에서의 핫 리로딩

---

**다음 학습**: [Scenario 06: 고급 기술](../scenario-06-advanced-techniques/) | **이전**: [Scenario 04: 카메라 시스템 수정](../scenario-04-camera-system/)

**⚡ 완료 예상 시간**: 14-21일 (하루 1-2시간 투자 기준)
