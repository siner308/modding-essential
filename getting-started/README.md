# 🚀 시작하기 - 게임 모딩 입문

게임 모딩의 세계에 오신 것을 환영합니다! 이 가이드는 안전하고 체계적인 모딩 학습을 위한 첫 단계입니다.

## 📖 목차

1. [게임 모딩이란?](#게임-모딩이란)
2. [필수 도구 설치](#필수-도구-설치)
3. [안전 가이드라인](#안전-가이드라인)
4. [법적 고려사항](#법적-고려사항)
5. [첫 번째 학습 준비](#첫-번째-학습-준비)

## 🎮 게임 모딩이란?

### 정의
**게임 모딩(Game Modding)**은 기존 게임의 동작, 외관, 기능을 수정하거나 확장하는 기술입니다.

### 모딩의 종류

#### 1. **콘텐츠 모딩** (입문자 친화적)
- 텍스처, 사운드, 모델 교체
- 게임 데이터 파일 수정
- 설정 파일 조작

#### 2. **스크립트 모딩** (중급)
- 게임 내 스크립트 언어 활용
- Lua, Python 등을 통한 로직 수정
- 게임 API 활용

#### 3. **바이너리 모딩** (고급) ← **이 저장소의 주 학습 대상**
- 실행 파일 직접 수정
- 메모리 패치 및 인젝션
- 리버스 엔지니어링

### 왜 바이너리 모딩을 배워야 할까?

```
✅ 모든 게임에 적용 가능 (모딩 도구가 없어도)
✅ 근본적인 게임 시스템 수정 가능
✅ 다른 모딩 기법의 기초가 됨
✅ 소프트웨어 개발 실력 향상
```

## 🛠️ 필수 도구 설치

### 1. 메모리 분석 도구

#### **Cheat Engine** (필수)
- **용도**: 메모리 스캔, 값 수정, 어셈블리 분석
- **다운로드**: [cheatengine.org](https://cheatengine.org/)
- **설치 주의사항**: 
  - 안티바이러스에서 오탐할 수 있음 (예외 처리 필요)
  - 설치 시 번들 소프트웨어 체크 해제

```powershell
# Windows에서 Chocolatey로 설치 (선택사항)
choco install cheatengine
```

#### **x64dbg** (권장)
- **용도**: 어셈블리 레벨 디버깅
- **다운로드**: [x64dbg.com](https://x64dbg.com/)
- **특징**: 오픈소스, 가벼움, 강력한 디버깅 기능

### 2. 헥스 에디터

#### **HxD** (Windows)
- **용도**: 바이너리 파일 편집
- **다운로드**: [mh-nexus.de/en/hxd](https://mh-nexus.de/en/hxd/)

#### **Hex Fiend** (macOS)
- **다운로드**: Mac App Store

### 3. 개발 환경

#### **Visual Studio Community** (권장)
- **용도**: C++ 모드 개발
- **다운로드**: [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)
- **필수 구성 요소**: 
  - C++ 데스크톱 개발 워크로드
  - Windows SDK

#### **Code::Blocks** (대안)
- **장점**: 가벼움, 무료
- **단점**: Visual Studio 대비 기능 제한

### 4. 보조 도구

#### **Process Monitor** (Microsoft)
- **용도**: 프로세스 모니터링
- **다운로드**: [Microsoft Sysinternals](https://docs.microsoft.com/en-us/sysinternals/)

#### **API Monitor**
- **용도**: API 호출 추적
- **다운로드**: [apimonitor.com](http://www.apimonitor.com/)

## 🛡️ 안전 가이드라인

### 기본 원칙

#### 1. **항상 백업하기**
```bash
# 게임 폴더 전체 백업
cp -r "C:/Games/YourGame" "C:/Backups/YourGame_backup"

# Steam 게임의 경우
# Steam → 라이브러리 → 게임 우클릭 → 속성 → 로컬 파일 → 파일 무결성 확인
```

#### 2. **격리된 환경에서 실험**
- 가상 머신 사용 권장
- 별도 게임 계정 사용
- 오프라인 모드에서 테스트

#### 3. **단계적 접근**
```
1. 읽기 전용 분석부터 시작
2. 임시 메모리 수정으로 테스트
3. 영구적 파일 수정은 마지막에
```

### 금지사항

#### ❌ **절대 하지 말 것**
- 온라인 멀티플레이어에서 치팅
- 게임 서버에 해를 끼치는 행위
- 상업적 목적의 무단 수정
- 저작권 침해 소지가 있는 배포

#### ⚠️ **주의사항**
- 안티치트 시스템이 있는 게임 조심
- 게임 업데이트 후 모드 재검증 필요
- 모드 적용 전 게임 무결성 확인

## 📜 법적 고려사항

### 합법적 모딩 범위

#### ✅ **일반적으로 허용되는 것들**
- 개인 사용 목적의 게임 수정
- 교육 및 학습 목적의 분석
- 접근성 향상을 위한 수정
- 버그 수정 및 성능 개선

#### ❓ **회색 지대**
- 게임 콘텐츠의 백업 및 보존
- 지역 제한 우회
- 게임 내 결제 시스템 우회

#### ❌ **명확히 불법인 것들**
- 게임 파일의 무단 배포
- DRM 우회 및 크랙
- 온라인 게임에서의 치팅
- 상업적 목적의 무단 수정

### 이용약관 확인

대부분의 게임은 다음과 같은 내용을 이용약관에 포함합니다:

```
"게임 클라이언트의 리버스 엔지니어링, 수정, 분해를 금지합니다."
```

하지만 많은 국가에서 **개인 학습 목적**은 예외로 인정합니다.

### 권장사항
1. 해당 게임의 이용약관 숙지
2. 개인 학습 목적임을 명확히
3. 수정 내용을 타인과 공유하지 않기
4. 의문사항은 법적 전문가와 상담

## 🎯 첫 번째 학습 준비

### 학습 환경 체크리스트

#### ✅ **필수 확인사항**
- [ ] Cheat Engine 설치 완료
- [ ] 테스트용 게임 준비 (Elden Ring 권장)
- [ ] 게임 백업 완료
- [ ] 가상 머신 또는 격리 환경 준비
- [ ] 기본 C++ 문법 이해

#### ✅ **권장 사항**
- [ ] x64dbg 설치
- [ ] Visual Studio 설치
- [ ] 헥스 에디터 설치
- [ ] 네트워크 차단 (온라인 게임의 경우)

### 테스트 게임 추천

#### **초급자용**
1. **Solitaire** - Windows 기본 게임
   - 간단한 구조
   - 안전한 테스트 환경
   - 기본 메모리 스캔 연습용

2. **Minesweeper** - Windows 기본 게임
   - 명확한 데이터 구조
   - 메모리 패턴 학습용

#### **중급자용**
1. **Elden Ring** - 이 저장소의 주 예제
   - 풍부한 모딩 커뮤니티
   - 다양한 모딩 기법 적용 가능
   - 오프라인 플레이 가능

2. **Skyrim** - 모딩 친화적 게임
   - 공식 모딩 지원
   - 광범위한 문서화
   - 안전한 실험 환경

### 기본 지식 확인

#### **C++ 기초**
```cpp
// 포인터와 메모리 주소 이해
int* ptr = &variable;
*ptr = 100;  // 포인터를 통한 값 수정

// 헥사데시말 표기법
int address = 0x401000;  // 16진수 주소
```

#### **어셈블리 기초**
```assembly
; 기본 명령어 이해
mov eax, 100    ; eax 레지스터에 100 대입
cmp eax, 0      ; eax와 0 비교
je label        ; 같으면 label로 점프
```

## 🔜 추천 학습 경로

이 프로젝트는 다양한 모딩 과정를 제공합니다. 최고의 학습 효과를 위해 다음 순서대로 진행하는 것을 권장합니다. 이 경로는 '무엇을' 할 수 있는지 먼저 경험하고 '어떻게' 작동하는지 나중에 파고드는 점진적 학습 방식입니다.

### 1단계: 도구 기반 모딩 (초급)
모딩의 재미와 가능성을 가장 쉽고 안전하게 체험하는 단계입니다.

1.  **[PAK 모딩 체험](../scenario-pak-modding/)**
    *   **학습 목표:** 언리얼 엔진 게임에서 `.pak` 파일을 이용한 모딩을 경험합니다.
    *   **핵심 기술:** UETools, 파일 기반 모드 설치.

2.  **[FromSoftware 게임 모딩](../scenario-fromsoftware-modding/)**
    *   **학습 목표:** Elden Ring과 같은 게임에서 커뮤니티 도구를 활용한 고급 모딩을 체험합니다.
    *   **핵심 기술:** Mod Engine 2, Smithbox, 파라미터 및 맵 에디팅.

### 2단계: 메모리 분석 및 리버스 엔지니어링 (중급 ~ 고급)
도구의 편리함 너머에 있는 핵심 원리를 이해하는 단계입니다. 1단계에서 "왜 이렇게 될까?"라는 궁금증을 가졌다면, 이제 그 해답을 찾을 차례입니다.

3.  **[게임 일시정지](../scenario-pause-game/)**
    *   **학습 목표:** Cheat Engine을 사용한 메모리 스캔과 어셈블리 분석의 기초를 다집니다.
    *   **핵심 기술:** 메모리 스캐닝, 포인터, 코드 인젝션 기초.

4.  **[FPS 언락](../scenario-unlock-fps/)**
    *   **학습 목표:** 동적으로 변하는 메모리 주소를 추적하고 패치하는 방법을 배웁니다.
    *   **핵심 기술:** 포인터 스캐닝, AOB 스크립팅.

5.  **이후 심화 과정**
    *   나머지 과정들(visual-effects, camera-system, mod-loader 등)을 통해 그래픽 후킹, 모드 로더 제작 등 특정 주제에 대한 깊이 있는 학습을 진행합니다.

### 학습 팁

#### **효과적인 학습 방법**
1. **실습 중심**: 이론만 보지 말고 직접 실험
2. **작은 목표**: 한 번에 하나씩 단계적 학습
3. **커뮤니티 활용**: 모딩 커뮤니티에서 정보 공유
4. **안전 우선**: 항상 백업하고 신중하게 진행

#### **문제 해결 방법**
1. **에러 로그 확인**: 모든 에러 메시지를 기록
2. **단계별 디버깅**: 문제 지점을 좁혀가며 분석
3. **커뮤니티 질문**: 구체적인 상황과 함께 질문
4. **문서 참조**: 공식 문서 및 레퍼런스 활용

## 🆘 도움이 필요하다면

### 추천 리소스
- **[OllyDbg 튜토리얼](https://tuts4you.com/)** - 어셈블리 분석
- **[Cheat Engine 튜토리얼](https://wiki.cheatengine.org/)** - 메모리 해킹
- **[x86 Assembly Guide](https://www.cs.virginia.edu/~evans/cs216/guides/x86.html)** - 어셈블리 기초

### 커뮤니티
- **Reddit**: r/RELounge, r/ReverseEngineering
- **Discord**: 각종 모딩 커뮤니티 서버
- **GitHub**: 오픈소스 모딩 프로젝트들

---

**🎯 준비가 되셨나요?** 

[PAK 모딩 체험](../scenario-pak-modding/)으로 첫 번째 실전 학습을 시작해보세요!
