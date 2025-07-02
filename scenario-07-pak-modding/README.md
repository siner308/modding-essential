# 🟢 Scenario 07: PAK 모딩 체험

**난이도**: 초급 | **학습 시간**: 3-5일 | **접근법**: UETools (PAK 기반)

## 🎯 학습 목표

UETools를 사용하여 언리얼 엔진 게임 모딩을 직접 체험하고, 모딩의 기본 개념을 이해합니다.

### 핵심 학습 포인트
- ✅ PAK 파일 시스템 이해
- ✅ 게임 모딩의 즉시 체험
- ✅ 안전한 모딩 방법 학습
- ✅ 콘솔 명령어 활용법
- ✅ Engine.ini 설정 파일 조작

## 📚 이론 학습

### PAK 기반 모딩이란?

PAK(Package) 파일은 언리얼 엔진에서 게임 에셋을 패키징하는 방법입니다.

```
언리얼 엔진 PAK 시스템
├── 게임 폴더/Content/Paks/
│   ├── GameName-WindowsNoEditor.pak    # 원본 게임 파일
│   └── ~mods/                          # 모드 폴더
│       ├── UETools_P.pak              # 개발자 도구
│       ├── MyMod_P.pak                # 커스텀 모드
│       └── TexturePack_P.pak          # 텍스처 팩
```

### 왜 PAK 모딩부터 시작하는가?

1. **즉시 체험**: 복잡한 설정 없이 바로 모딩 효과 확인
2. **안전성**: 게임 파일 직접 수정 없음
3. **되돌리기 쉬움**: 파일 삭제만으로 원상복구
4. **학습 효과**: 모딩의 핵심 개념 체험

## 🛠️ 실습 도구

### 필수 도구
- **UETools**: Lies of P용 개발자 도구 모드
- **메모장**: Engine.ini 설정 편집
- **Windows 탐색기**: 파일 관리

### 선택 도구
- **FModel**: PAK 파일 내용 확인 (고급)
- **UnrealPak**: PAK 파일 생성 도구 (고급)

## 🔍 실습 과정

### 1단계: 게임 준비
```bash
# Lies of P가 설치된 폴더 찾기
Steam/steamapps/common/Lies of P/
├── LiesofP.exe
├── Engine/
└── LiesofP/
    └── Content/
        └── Paks/           # 여기가 모드 설치 위치
```

### 2단계: UETools 설치
```bash
# 1. ~mods 폴더 생성
mkdir "Steam/steamapps/common/Lies of P/LiesofP/Content/Paks/~mods"

# 2. UETools 파일 복사
~mods/
└── UETools-WindowsNoEditor_p.pak
```

### 3단계: 첫 모딩 체험
1. **게임 실행**
2. **F1 키 눌러보기** → 스크린샷 촬영됨
3. **~ 키 눌러서 콘솔 열기**
4. **명령어 입력해보기**:
   ```
   god          # 무적 모드
   fly          # 비행 모드
   slomo 0.5    # 슬로우 모션
   ```

### 4단계: Engine.ini 커스터마이징

**파일 위치**: `%LOCALAPPDATA%/LiesofP/Saved/Config/WindowsNoEditor/Engine.ini`

```ini
[/Game/UETools_Implemintation/Progressive/Settings.Settings_C]
# 스크린샷 해상도 변경
HighResScreenshotResolution=1920x1080

# 자동 실행 명령어 설정
AutoExecCommand_01=god
AutoExecCommand_02=fly
AutoExecCommand_03=UETools_Help

# 단축키 커스터마이징
DebugMapping_Screenshot=F12
DebugMapping_ToggleFullScreen=F11
```

### 5단계: 고급 기능 탐험
```bash
# 디버그 정보 표시
ShowDebug AI
ShowDebug Collision
ShowDebug Rendering

# 성능 모니터링
stat fps
stat unit
stat memory

# 게임 상태 조작
pause
slomo 2.0
teleport 0 0 100
```

## 📋 체크리스트

### 기본 체험
- [ ] UETools 설치 완료
- [ ] F1 스크린샷 기능 확인
- [ ] 콘솔 명령어 5개 이상 테스트
- [ ] god 모드로 무적 체험
- [ ] fly 모드로 자유 비행 체험

### 설정 커스터마이징
- [ ] Engine.ini 파일 찾기
- [ ] 자동 실행 명령어 설정
- [ ] 단축키 변경해보기
- [ ] 스크린샷 해상도 조정

### 안전 확인
- [ ] 원본 게임 파일 무손상 확인
- [ ] 모드 제거 후 정상 작동 확인
- [ ] 다른 모드와 함께 사용해보기

## 🎓 학습 정리

### 배운 개념들
1. **PAK 시스템**: 언리얼 엔진의 에셋 패키징 방법
2. **모드 우선순위**: 파일명과 폴더 구조의 중요성
3. **콘솔 명령어**: 개발자 도구의 활용법
4. **설정 파일**: Engine.ini를 통한 커스터마이징
5. **안전한 모딩**: 원본 파일 보호 방법

### 다음 단계 준비
- ✅ 모딩의 기본 개념 이해
- ✅ 파일 시스템 구조 파악
- ✅ 안전한 모딩 방법 체득
- 🔄 바이너리 모딩 학습 준비 (Scenario 02)

## 🔗 관련 자료

- [UETools 공식 가이드](../../reference/unreal-engine-modding.md#uetools---개발자-도구)
- [PAK 모딩 심화 학습](../../reference/unreal-engine-modding.md#pak-기반-모드-로더-시스템)
- [언리얼 엔진 모딩 전체 가이드](../../reference/unreal-engine-modding.md)

## 💡 트러블슈팅

### 자주 발생하는 문제

**Q: 콘솔이 열리지 않아요**
A: Engine.ini에서 `bConstructConsole=True` 설정 확인

**Q: 명령어가 작동하지 않아요**  
A: UETools가 올바르게 설치되었는지 확인 (~mods 폴더 위치)

**Q: 게임이 크래시돼요**
A: 모든 PAK 파일을 제거하고 게임 무결성 검사 실행

**Q: 설정이 저장되지 않아요**
A: Engine.ini 파일의 읽기 전용 속성 해제 확인

---

**다음 학습**: [Scenario 08: FromSoftware 게임 모딩](../scenario-08-fromsoftware-modding/) - FromSoftware 게임 모딩의 세계로