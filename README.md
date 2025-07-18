# 🎮 Modding Essential - 게임 모딩 학습 가이드

실전 예제를 통해 배우는 게임 모딩 기술 학습 저장소

## 📚 학습 방식

이 저장소는 **과정 기반 학습**을 통해 게임 모딩 기술을 단계적으로 습득할 수 있도록 구성되었습니다. 각 과정는 실제 게임 모드를 예시로 하여 이론과 실습을 병행합니다.

## 🚀 시작하기

### 1. 필수 준비사항
- [getting-started/](./getting-started/) 폴더에서 시작
- 도구 설치 및 환경 설정
- 안전한 모딩을 위한 기본 지식

### 2. 추천 학습 경로

이 프로젝트는 최고의 학습 효과를 위해 다음의 점진적인 학습 경로를 추천합니다.

```
🟢 입문자
PAK 모딩 체험 → 게임 일시정지 구현 → FPS 제한 해제 → FromSoftware 게임 모딩
    ↓
🟡 중급자
시각 효과 수정 → 카메라 시스템 수정 → PAK 모드 로더 시스템 → 몬스터 대전 시스템
    ↓
🔴 고급자
DLL 모드 로더 시스템 → 고급 모딩 기법 → 최종 프로젝트
```

## 📖 과정별 학습 내용

### 🟢 초급 (Beginner)

#### [PAK 모딩 체험](./scenario-pak-modding/)
- **목표**: UETools로 언리얼 엔진 게임 모딩 첫 경험
- **핵심 기술**: PAK 파일 설치, 콘솔 명령어, Engine.ini 설정
- **예제 게임**: Lies of P, 기타 언리얼 엔진 게임
- **학습 시간**: 3-5일
- **접근법**: UETools (즉시 체험 가능)

#### [게임 일시정지 구현](./scenario-pause-game/)
- **목표**: 온라인 게임에 일시정지 기능 추가
- **핵심 기술**: 메모리 스캔, 어셈블리 기초, 조건부 점프
- **예제 게임**: Elden Ring, Dark Souls 시리즈
- **학습 시간**: 1-2주
- **접근법**: 바이너리 모딩 (본격적인 모딩 시작)

#### [FPS 제한 해제](./scenario-unlock-fps/)
- **목표**: 60FPS 제한을 120FPS, 144FPS 또는 무제한으로 해제
- **핵심 기술**: 부동소수점 수치 조작, 메모리 패치, 성능 최적화
- **예제 게임**: EldenRing, Unity/언리얼 게임들
- **학습 시간**: 1-2주
- **접근법**: 바이너리 모딩 (수치 조작)

#### [FromSoftware 게임 모딩](./scenario-fromsoftware-modding/)
- **목표**: Dantelion 엔진 기반 게임의 전문 모딩 도구 활용
- **핵심 기술**: DSMapStudio, Smithbox, Mod Engine 2, 파라미터 편집
- **예제 게임**: Elden Ring, Dark Souls 시리즈, Sekiro
- **학습 시간**: 2-3주
- **접근법**: 커뮤니티 도구 기반

### 🟡 중급 (Intermediate)

#### [시각 효과 수정](./scenario-visual-effects/)
- **목표**: 게임의 시각적 효과를 실시간으로 수정하기
- **핵심 기술**: DirectX 후킹, 셰이더 조작, 포스트 프로세싱
- **예제**: ReShade 스타일 인젝터, 커스텀 필터
- **학습 시간**: 2-3주
- **접근법**: 그래픽스 파이프라인 조작

#### [카메라 시스템 수정](./scenario-camera-system/)
- **목표**: FOV 조정, 프리 카메라, 카메라 동작 변경
- **핵심 기술**: 3D 수학, 함수 후킹, 동적 패치
- **예제 게임**: FPS/TPS 게임들
- **학습 시간**: 2-3주
- **접근법**: 바이너리 모딩 + 3D 수학

#### [PAK 모드 로더 시스템](./scenario-pak-loader/)
- **목표**: Interpose 같은 PAK 기반 모드 로더 이해하기
- **핵심 기술**: PAK 우선순위, 모듈화 설계, 호환성 관리
- **예제**: Interpose Mod Loader, AISpawner.pak
- **학습 시간**: 1-2주
- **접근법**: PAK 모딩 (고급)

#### [몬스터 대전 시스템](./scenario-monster-battle/)
- **목표**: AI 몬스터/보스 간 대전 시스템 구축
- **핵심 기술**: AI 팩션 조작, 이벤트 스크립팅, 관전 모드
- **예제 게임**: Elden Ring, Dark Souls, Unity/언리얼 게임
- **학습 시간**: 3-4주
- **접근법**: 멀티 플랫폼 (FromSoft + 바이너리)

### 🔴 고급 (Advanced)

#### [DLL 모드 로더 시스템](./scenario-mod-loader/)
- **목표**: 범용 DLL 기반 모드 로더 시스템 구축
- **핵심 기술**: DLL 인젝션, 프록시 기법, 아키텍처 설계
- **예제**: EldenRing ModLoader, 기타 게임 로더들
- **학습 시간**: 3-4주
- **접근법**: 바이너리 모딩 (고급)

#### [고급 모딩 기법](./scenario-advanced-techniques/)
- **목표**: 안티 디버그 우회, 코드 동굴, 고급 후킹
- **핵심 기술**: 리버스 엔지니어링, 보안 우회, 언패킹
- **예제**: 보호 기법이 적용된 게임들, 패킹된 실행파일
- **학습 시간**: 4-6주
- **접근법**: 바이너리 모딩 (전문가)

## 🛠️ 지원 자료

### [Reference](./reference/)
- 어셈블리 퀵 레퍼런스
- 메모리 레이아웃 가이드
- 자주 사용되는 패턴 모음
- 도구 비교 및 사용법
- 문제 해결 가이드

### [Projects](./projects/)
- 실전 프로젝트 템플릿
- 간단한 트레이너 만들기
- 게임 오버레이 시스템
- 모드 프레임워크 구축

### [Resources](./resources/)
- 추천 도서 및 자료
- 커뮤니티 링크
- 법적 고려사항
- 윤리적 모딩 가이드

## 🎯 학습 목표별 추천 경로

### 🚀 빠른 모딩 체험 (즉시 시작)
`getting-started` → `scenario-pak-modding` → UETools 활용

### 🎮 게임 트레이너 만들기
`getting-started` → `scenario-pause-game` → `scenario-unlock-fps` → `projects/simple-trainer`

### 📦 PAK 모딩 마스터
`scenario-pak-modding` → `scenario-pak-loader` → `reference/unreal-engine-modding.md`

### ⚔️ FromSoftware 게임 모딩
`scenario-fromsoftware-modding` → `reference/unreal-engine-modding.md`

### 🥊 AI 대전 시스템 개발
`scenario-fromsoftware-modding` → `scenario-monster-battle`

### 🎨 시각적 모드 개발
`getting-started` → `scenario-visual-effects`

### 🏗️ 모드 시스템 구축
`getting-started` → `scenario-mod-loader` → `scenario-advanced-techniques` → `projects/mod-framework`

## 📋 각 과정 구성

모든 과정는 다음과 같은 5단계 학습 구조를 따릅니다:

1. **📖 이론 학습** - 기본 개념과 원리 이해
2. **🔧 도구 사용** - 필요한 도구들의 사용법 습득
3. **🔍 코드 분석** - 실제 모드 코드 분석 및 이해
4. **⚡ 실습 구현** - 단계별 구현 및 테스트
5. **🚀 응용 연습** - 심화 과제 및 변형 실습

## 🎮 지원 게임 예제

### 현재 포함된 게임들
- **Elden Ring** (FromSoftware) - 기본 모딩 기법
- **Dark Souls 시리즈** - 유사한 엔진 기법
- **Skyrim** - 광범위한 모딩 생태계
- **Minecraft** - Java 기반 모딩
- **기타 언리얼/유니티 게임들**

### 추가 예정
- **GTA 시리즈** - 오픈월드 게임 모딩
- **Counter-Strike** - 멀티플레이어 게임 모딩
- **인디 게임들** - 다양한 엔진별 기법

## ⚠️ 중요 공지

### 안전성 우선
- 모든 실습은 **안전한 환경**에서 진행
- **게임 백업** 필수
- **싱글플레이어 모드**에서만 테스트
- **온라인 게임**에서의 치팅 절대 금지

### 법적 고려사항
- 개인 학습 목적으로만 사용
- 상업적 이용 금지
- 게임 개발사의 이용약관 준수
- 저작권 침해 금지

### 윤리적 모딩
- 다른 플레이어에게 피해를 주는 모드 제작 금지
- 게임 경험을 향상시키는 건전한 모드 개발 지향
- 커뮤니티와 지식 공유

## 🤝 기여하기

이 프로젝트는 오픈소스이며 모든 기여를 환영합니다:

- 새로운 과정 추가
- 게임별 예제 코드 기여  
- 문서 개선 및 번역
- 오류 신고 및 수정

## 📬 연락처

- **Issues**: GitHub Issues를 통한 질문 및 제안
- **Discussions**: 학습 관련 토론 및 정보 공유
- **Pull Requests**: 직접적인 기여

## 📜 라이선스

이 프로젝트는 교육 목적으로 제작되었으며, MIT 라이선스 하에 배포됩니다.

---

**🎯 목표**: 안전하고 윤리적인 게임 모딩 기술의 학습과 발전

**⭐ 시작하기**: [getting-started](./getting-started/) 폴더부터 시작하세요!
