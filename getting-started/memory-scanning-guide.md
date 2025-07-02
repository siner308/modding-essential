# 🔍 메모리 스캔 방법 완전 가이드

**EldenRing 모드 코드를 실제로 분석하면서 메모리 스캔 과정을 학습합니다**

## 📖 메모리 스캔이란?

메모리 스캔은 실행 중인 프로그램의 메모리에서 특정 값이나 패턴을 찾는 기법입니다.

### 기본 개념
```
게임 프로세스 메모리 구조:
├── Code Section (.text) - 실행 코드
├── Data Section (.data) - 초기화된 데이터
├── BSS Section (.bss) - 미초기화 데이터
├── Heap - 동적 할당 메모리
└── Stack - 함수 호출 스택
```

## 🛠️ 필수 도구

### 1. Cheat Engine
- **다운로드**: https://cheatengine.org/
- **용도**: 메모리 스캔, 값 변경, 코드 분석
- **장점**: 무료, 강력한 기능, 대용량 커뮤니티

### 2. x64dbg
- **다운로드**: https://x64dbg.com/
- **용도**: 디스어셈블리, 디버깅, 코드 분석
- **장점**: 오픈소스, 플러그인 지원

### 3. Process Hacker
- **다운로드**: https://processhacker.sourceforge.io/
- **용도**: 프로세스 모니터링, 메모리 맵 확인
- **장점**: 시스템 수준 정보 제공

## 🎮 EldenRing PauseTheGame 모드 분석

### 실제 코드 살펴보기

EldenRing PauseTheGame 모드의 핵심 AOB(Array of Bytes) 패턴:
```cpp
std::string aob = "0f 84 ? ? ? ? c6 ? ? ? ? ? 00 ? 8d ? ? ? ? ? ? 89 ? ? 89 ? ? ? 8b ? ? ? ? ? ? 85 ? 75";
```

### 패턴 해독
```
원본 어셈블리:
0F 84 XX XX XX XX    - JE (Jump if Equal) 명령어
C6 XX XX XX XX XX 00 - MOV byte ptr, 0

핵심 부분:
- 0F 84: JE 명령어 (조건부 점프)
- ?: 와일드카드 (가변적인 주소값)
- 고정 바이트들: 패턴 식별용
```

## 📐 실제 Cheat Engine으로 스캔 해보기

### 단계 1: 게임 프로세스 연결
```bash
1. Cheat Engine 실행
2. "Open Process" 클릭
3. "eldenring.exe" 선택
4. Memory Scan 준비
```

### 단계 2: AOB 스캔 실행
```bash
1. Scan Type: "Array of bytes" 선택
2. Value에 입력:
   0F 84 ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? 00
3. "First Scan" 실행
4. 결과 확인 (보통 1-3개 매치)
```

### 단계 3: 패치 지점 확인
```bash
스캔 결과 주소: 예) 0x140A2B5C0
패치 위치: 주소 + 1 (offset) = 0x140A2B5C1
원본 바이트: 84 (JE - Jump if Equal)
패치 바이트: 85 (JNE - Jump if Not Equal)
```

## 🔧 실습: 직접 메모리 스캔 해보기

### 실습 1: Cheat Engine으로 HP 찾기
```bash
# 단계별 과정
1. 게임 시작, 현재 HP 확인 (예: 600)
2. Cheat Engine에서 "600" 검색
3. 데미지를 받아서 HP 변경 (예: 550)
4. "Changed value to 550" 재검색
5. 결과가 1-2개 나올 때까지 반복
6. 찾은 주소의 값을 수정해서 테스트
```

### 실습 2: 게임 상태 변수 찾기
```bash
# 일시정지 관련 변수 찾기
1. 게임이 정상 작동할 때: 값 0 스캔
2. 메뉴를 열어서 일시정지 상태로: 값 1 스캔
3. 다시 게임으로 돌아가서: Changed to 0
4. 반복하여 일시정지 플래그 찾기
```

### 실습 3: AOB 패턴 직접 만들기
```bash
# x64dbg를 사용한 패턴 생성
1. x64dbg로 eldenring.exe 연결
2. 원하는 함수 찾기 (예: UpdateGame)
3. 함수 시작 부분의 바이트 확인
4. 고정 부분과 가변 부분 구분
5. 와일드카드(?)를 사용한 패턴 생성
```

## 📊 메모리 스캔 성공률 높이는 팁

### 1. 효과적인 패턴 설계
```cpp
// 좋은 패턴 예시
"48 89 5C 24 ? 57 48 83 EC 20 48 8B F9"
- 충분히 긴 패턴 (12+ 바이트)
- 고유한 시퀀스
- 적절한 와일드카드 사용

// 피해야 할 패턴
"48 89"
- 너무 짧음 (수백 개 매치)
- 일반적인 명령어 조합
```

### 2. 다양한 스캔 방법
```bash
Method 1: 직접 값 스캔
- HP, MP, 경험치 등 숫자 값
- 변화를 추적하기 쉬움

Method 2: 포인터 스캔
- 값의 주소를 가리키는 포인터 찾기
- 게임 재시작 시에도 유효

Method 3: 구조체 스캔
- 연관된 값들의 패턴 찾기
- 예: HP(4바이트) + MP(4바이트) + Level(4바이트)
```

## ⚠️ 메모리 스캔 시 주의사항

### 1. 메모리 보호
```cpp
// 메모리 쓰기 전 보호 해제 필요
DWORD oldProtect;
VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
// 패치 실행
VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
```

### 2. 게임 업데이트 대응
```bash
문제: 게임 업데이트 시 패턴 변경
해결책:
1. 여러 패턴 준비 (버전별)
2. 더 일반적인 패턴 사용
3. 시그니처 자동 업데이트 시스템
```

---

**💡 핵심 기억사항**:
메모리 스캔은 강력한 도구이지만, 항상 **합법적이고 윤리적인** 목적으로만 사용해야 합니다!