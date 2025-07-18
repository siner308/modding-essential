# 🟡 PAK 모드 로더 시스템

**난이도**: 중급 | **학습 시간**: 1-2주 | **접근법**: PAK 모딩 (고급)

## 🎯 학습 목표

Interpose Mod Loader와 같은 커뮤니티 개발 PAK 기반 모드 로더 시스템을 이해하고, 고급 PAK 모딩 기법을 익힙니다.

### 핵심 학습 포인트
- ✅ PAK 파일 우선순위 시스템 이해
- ✅ 모듈화된 모드 설계 원칙
- ✅ 모드 호환성 관리 기법
- ✅ 동적 PAK 로딩 시스템
- ✅ 커뮤니티 모딩 표준 학습

## 📚 이론 학습

### PAK 기반 모드 로더란?

```
Interpose Mod Loader 구조
├── Z_ModLoader_P.pak           # 핵심 로더 (최고 우선순위)
├── LogicMods/                  # 부가 기능 모듈
│   ├── Z_DebugLog_P.pak       # 디버그 로그
│   ├── Z_ModButtons_P.pak     # UI 제어
│   └── Z_ModListMenu_P.pak    # 모드 관리
└── ~mods/                      # 일반 모드들
    ├── AISpawner.pak          # AI 스폰 제어
    ├── TextureMod_P.pak       # 텍스처 교체
    └── CustomUI_P.pak         # UI 모드
```

### 우선순위 기반 로딩 시스템

언리얼 엔진은 PAK 파일을 **알파벳 순서**로 로드하며, 나중에 로드된 파일이 우선권을 가집니다.

```bash
로드 순서 (알파벳 순):
1. A_CoreMod_P.pak          # 첫 번째 로드
2. B_UIFramework_P.pak      # 두 번째 로드  
3. MyCustomMod_P.pak        # 세 번째 로드
4. Z_ModLoader_P.pak        # 마지막 로드 (최고 우선순위!)
```

**Z_ 접두사 전략**:
- `Z_`로 시작하는 파일이 가장 나중에 로드됨
- 따라서 모드 로더는 `Z_`를 사용하여 최고 우선순위 확보
- 다른 모든 모드들을 관리하고 제어할 수 있음

## 🔍 실습 분석: Interpose Mod Loader

### 1단계: 파일 구조 분석

```bash
Interpose Lies of P Mod Loader/
├── ~mods/
│   └── Z_ModLoader_P.pak          # 568 KB - 핵심 로더
└── LogicMods/
    ├── Z_DebugLog_P.pak          # 234 KB - 디버그 기능
    ├── Z_ModButtons_P.pak        # 156 KB - UI 컨트롤
    └── Z_ModListMenu_P.pak       # 289 KB - 모드 목록
```

### 2단계: 설치 및 테스트

```bash
# 1. 기본 설치
복사: ~mods/Z_ModLoader_P.pak → 게임폴더/Content/Paks/~mods/

# 2. 부가 기능 설치 (선택)
복사: LogicMods/* → 게임폴더/Content/Paks/LogicMods/

# 3. 테스트 모드 추가
복사: AISpawner.pak → 게임폴더/Content/Paks/~mods/
```

### 3단계: 로드 순서 실험

다양한 파일명으로 로드 순서 테스트:

```bash
# 실험용 PAK 파일들 (가상)
01_FirstMod_P.pak        # 첫 번째 그룹
02_SecondMod_P.pak       
A_HighPriority_P.pak     # 두 번째 그룹
B_MediumPriority_P.pak   
MyNormalMod_P.pak        # 세 번째 그룹 (기본)
TestMod_P.pak            
Z_ModLoader_P.pak        # 네 번째 그룹 (최고 우선순위)
Z_SystemMod_P.pak        
```

## 🛠️ 고급 PAK 모딩 실습

### 모듈화된 모드 설계

하나의 큰 모드를 여러 PAK으로 분리하는 방법:

```bash
MyComplexMod/
├── Core_MyMod_P.pak        # 핵심 로직 (필수)
├── UI_MyMod_P.pak          # 사용자 인터페이스 (선택)
├── Assets_MyMod_P.pak      # 그래픽/사운드 (선택)
└── Config_MyMod_P.pak      # 설정 파일 (선택)

설치 순서:
1. Core (항상 필요)
2. UI (사용자가 원할 때만)
3. Assets (고화질을 원할 때만)
4. Config (커스터마이징 시에만)
```

### 호환성 매트릭스 만들기

```text
모드 호환성 차트:
┌─────────────────┬──────────┬──────────┬──────────┐
│     모드명      │ MyModA   │ MyModB   │ MyModC   │
├─────────────────┼──────────┼──────────┼──────────┤
│ MyModA          │    -     │    ✅    │    ❌    │
│ MyModB          │    ✅    │    -     │    ⚠️    │  
│ MyModC          │    ❌    │    ⚠️    │    -     │
└─────────────────┴──────────┴──────────┴──────────┘

범례:
✅ = 완전 호환
⚠️ = 부분 호환 (주의 필요)
❌ = 호환 불가 (충돌 발생)
```

## 📋 실전 실습 과제

### 과제 1: 우선순위 실험
1. 동일한 파일을 다른 이름으로 복사
2. 로드 순서 확인
3. 파일명 변경하여 우선순위 조작

### 과제 2: 모드 충돌 시뮬레이션  
1. 같은 기능을 하는 모드 2개 설치
2. 충돌 상황 관찰
3. 우선순위로 충돌 해결

### 과제 3: 커스텀 모드 패키징
1. FModel로 기존 PAK 내용 확인
2. 간단한 텍스처 수정
3. UnrealPak으로 새 PAK 생성

## 🔧 고급 도구 활용

### FModel 사용법
```bash
# PAK 파일 내용 확인
FModel.exe
1. 게임 경로 설정
2. PAK 파일 선택  
3. 내용 탐색 및 추출
```

### UnrealPak 명령어
```bash
# PAK 파일 생성
UnrealPak.exe MyMod_P.pak -create=filelist.txt -compress

# filelist.txt 예제
"MyTexture.uasset" "../../../Content/Textures/MyTexture.uasset"
"MyMaterial.uasset" "../../../Content/Materials/MyMaterial.uasset"
```

## 📊 성능 및 최적화

### PAK 파일 크기 최적화
```bash
압축 옵션 비교:
- 압축 없음: 10.5 MB (빠른 로딩)
- 기본 압축: 3.2 MB (균형)  
- 최대 압축: 2.1 MB (느린 로딩)

권장사항:
✅ 텍스처 모드: 최대 압축
✅ 로직 모드: 기본 압축
✅ 시스템 모드: 압축 없음
```

### 메모리 사용량 모니터링
```bash
# 게임 내에서 메모리 확인
stat memory
stat streaming

# 모드 로딩 전후 비교
Before: 2.1 GB
After:  2.8 GB (+700 MB)
```

## 🎓 학습 정리

### 핵심 개념 정리
1. **우선순위 시스템**: Z_ > A-Y > 숫자 > 일반명
2. **모듈화 설계**: 기능별 PAK 분리
3. **호환성 관리**: 충돌 방지 및 의존성 해결
4. **커뮤니티 표준**: 공통 규칙으로 생태계 구축

### 실무 적용 포인트
- 📦 대규모 모드의 모듈화 전략
- 🔄 버전 업데이트 시 호환성 유지
- 🛡️ 사용자 경험을 해치지 않는 안전한 로딩
- 🌐 커뮤니티와의 표준 공유

## 🔗 관련 자료

- [PAK 기반 모드 로더 시스템 상세](../../reference/unreal-engine-modding.md#pak-기반-모드-로더-시스템)
- [언리얼 엔진 PAK 시스템 이해](../../reference/unreal-engine-modding.md#주요-모딩-접근법)
- [모딩 접근법 비교: PAK vs 다른 방법들](../../reference/modding-approaches.md)

## 💡 다음 단계

이제 PAK 모딩의 고급 기법을 익혔으니, 다음 과정로 넘어가세요:

- **몬스터 대전**: [몬스터 대전 시스템 구현](../scenario-monster-battle/)

---

**이전 학습**: [FromSoftware 게임 모딩](../scenario-fromsoftware-modding/) | **다음 학습**: [몬스터 대전 시스템 구현](../scenario-monster-battle/)