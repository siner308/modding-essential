# 🛠️ 모딩 도구 설치 및 설정 가이드

게임 모딩에 필요한 필수 도구들의 설치와 기본 설정 방법을 안내합니다.

## 📋 목차

1. [Cheat Engine 설치 및 설정](#cheat-engine)
2. [x64dbg 설치 및 설정](#x64dbg)
3. [Visual Studio 설정](#visual-studio)
4. [헥스 에디터 설정](#헥스-에디터)
5. [보조 도구들](#보조-도구들)
6. [환경 설정 검증](#환경-설정-검증)

## 🔍 Cheat Engine

### 설치

#### Windows
1. **공식 사이트에서 다운로드**
   ```
   https://cheatengine.org/
   ```

2. **설치 시 주의사항**
   - 안티바이러스 소프트웨어에서 오탐 경고가 나올 수 있음
   - 설치 중 번들 소프트웨어 설치 체크 해제
   - "McAfee WebAdvisor" 등 불필요한 소프트웨어 거부

3. **안티바이러스 예외 처리**
   ```bash
   # Windows Defender 예외 추가 (관리자 권한으로 실행)
   powershell -Command "Add-MpPreference -ExclusionPath 'C:\Program Files\Cheat Engine 7.x'"
   ```

### 기본 설정

#### 1. **첫 실행 설정**
```
Cheat Engine 실행 → Settings → General
├── "Confirm exit" 체크 (실수로 종료 방지)
├── "Save settings on exit" 체크
└── "Check for updates" 체크 해제 (선택사항)
```

#### 2. **디버거 설정**
```
Settings → Debugger Options
├── "Break and trace" 활성화
├── "VEH Debugger" 선택 (호환성 좋음)
└── "Kernel debugger" 비활성화 (일반 사용자용)
```

#### 3. **어셈블리 설정**
```
Settings → Disassembler
├── "Syntax" → Intel (x86 표준)
├── "Show module+offset" 활성화
└── "Show bytes" 활성화
```

### 기본 사용법 테스트

#### **튜토리얼 실행**
```
Cheat Engine → Help → Tutorial
```

**Step 1-3 까지는 반드시 완료하세요!**
- Step 1: 값 찾기 및 수정
- Step 2: 값 변화 추적
- Step 3: 플로트 값 조작

## 🔧 x64dbg

### 설치

#### Windows
1. **GitHub에서 다운로드**
   ```
   https://github.com/x64dbg/x64dbg/releases
   ```

2. **압축 해제**
   ```bash
   # 원하는 위치에 압축 해제 (예: C:\Tools\x64dbg)
   ```

3. **바로가기 생성**
   - `x32dbg.exe` (32비트 프로그램용)
   - `x64dbg.exe` (64비트 프로그램용)

### 기본 설정

#### 1. **외관 설정**
```
Options → Appearance
├── Color Scheme → "Dark" (선택사항)
├── Font → "Consolas" 또는 "Courier New"
└── Font Size → 10-12pt
```

#### 2. **디스어셈블러 설정**
```
Options → Disassembler
├── "Syntax" → Intel
├── "Opcodes" 표시 활성화
└── "Memory addresses" 표시 활성화
```

#### 3. **플러그인 설정**
```
기본 플러그인들 활성화:
├── "Scylla" (Import reconstruction)
├── "OllyDumpEx" (Process dumping)
└── "xAnalyzer" (고급 분석, 선택사항)
```

### 기본 사용법

#### **간단한 프로그램 분석 테스트**
```bash
# Windows Calculator 분석 예제
1. Calculator.exe 실행
2. x64dbg에서 "File → Attach"
3. Calculator 프로세스 선택
4. "Debug → Run" (F9)
```

## 💻 Visual Studio

### 설치

#### Visual Studio Community (무료)
1. **다운로드**
   ```
   https://visualstudio.microsoft.com/vs/community/
   ```

2. **워크로드 선택**
   ```
   필수:
   ├── "C++를 사용한 데스크톱 개발"
   ├── "Windows 10/11 SDK" (최신 버전)
   └── "CMake tools for C++" (선택사항)
   ```

### 프로젝트 설정

#### **DLL 프로젝트 템플릿**

```cpp
// DllMain.cpp 기본 템플릿
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // 모드 초기화 코드
            DisableThreadLibraryCalls(hinstDLL);
            CreateThread(0, 0, MainThread, 0, 0, NULL);
            break;
        case DLL_PROCESS_DETACH:
            // 정리 코드
            break;
    }
    return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // 실제 모드 로직
    return 0;
}
```

#### **프로젝트 속성 설정**
```
Configuration Properties
├── General
│   ├── Configuration Type → Dynamic Library (.dll)
│   └── Platform Toolset → v143 (Visual Studio 2022)
├── C/C++
│   ├── General → Additional Include Directories
│   └── Code Generation → Runtime Library → MT (정적 링크)
└── Linker
    ├── General → Output File → $(OutDir)$(TargetName)$(TargetExt)
    └── Input → Additional Dependencies → 필요한 라이브러리들
```

### 유용한 확장 프로그램

#### **추천 Extensions**
```
Extensions → Manage Extensions
├── "Hex Editor" - 바이너리 파일 편집
├── "Assembly Syntax Highlighting" - 어셈블리 하이라이팅
└── "GitLens" - Git 통합 (선택사항)
```

## 📝 헥스 에디터

### HxD (Windows)

#### 설치 및 설정
```
1. https://mh-nexus.de/en/hxd/ 에서 다운로드
2. 설치 후 실행
3. Options → Font → "Courier New" 또는 "Consolas"
4. View → Bytes per row → 16 (표준)
```

#### 기본 사용법
```
파일 열기: File → Open
├── 바이너리 모드로 자동 열림
├── Ctrl+G: 특정 오프셋으로 이동
├── Ctrl+F: 바이트 패턴 검색
└── Ctrl+H: 바이트 교체
```

### Hex Fiend (macOS)

#### 설치
```bash
# Mac App Store에서 설치
# 또는 Homebrew 사용
brew install --cask hex-fiend
```

## 🔍 보조 도구들

### Process Monitor

#### 설치 및 설정
```
1. Microsoft Sysinternals에서 다운로드
2. 압축 해제 후 ProcMon.exe 실행
3. Filter → Process Name contains → 게임 이름
4. Options → Always On Top 활성화
```

#### 모니터링 설정
```
Process Monitor 필터:
├── Process Name: [게임명].exe
├── Operation: Process and Thread Activity
└── Result: SUCCESS만 표시
```

### API Monitor

#### 설치
```
1. http://www.apimonitor.com/ 에서 다운로드
2. 설치 후 실행
3. File → Monitor New Process
4. 게임 실행 파일 선택
```

#### 유용한 API 모니터링
```
추천 API 카테고리:
├── Kernel32.dll (파일/메모리 작업)
├── User32.dll (윈도우/입력 처리)
├── D3D11.dll (DirectX 그래픽)
└── OpenGL32.dll (OpenGL 그래픽)
```

## ✅ 환경 설정 검증

### 1. Cheat Engine 테스트

#### **기본 기능 확인**
```bash
# Windows Calculator로 테스트
1. Calculator 실행
2. Cheat Engine 실행
3. Process List에서 Calculator 선택
4. "Tutorial"로 기본 기능 테스트
```

#### **성공 기준**
- [ ] 프로세스 어태치 성공
- [ ] 메모리 스캔 가능
- [ ] 값 수정 및 확인 가능
- [ ] 어셈블리 뷰 정상 작동

### 2. x64dbg 테스트

#### **디버깅 기능 확인**
```bash
# 간단한 프로그램 디버깅
1. Notepad.exe 실행
2. x64dbg로 어태치
3. F9 (Run) 후 F12 (Pause)
4. 어셈블리 코드 확인
```

#### **성공 기준**
- [ ] 프로세스 어태치 성공
- [ ] 중단점 설정 가능
- [ ] 레지스터 값 확인 가능
- [ ] 스택 뷰 정상 작동

### 3. Visual Studio 테스트

#### **DLL 컴파일 테스트**
```cpp
// test.cpp - 간단한 DLL 테스트
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
    MessageBox(NULL, L"DLL Loaded!", L"Test", MB_OK);
    return TRUE;
}
```

#### **컴파일 및 테스트**
```bash
1. 새 DLL 프로젝트 생성
2. 위 코드 작성
3. F5 또는 Ctrl+Shift+B로 컴파일
4. 생성된 DLL 파일 확인
```

### 4. 통합 테스트

#### **실제 게임과 연동**
```bash
# Elden Ring을 이용한 통합 테스트 (선택사항)
1. Elden Ring 실행 (오프라인 모드)
2. Cheat Engine으로 프로세스 어태치
3. 플레이어 HP 값 검색
4. 값 발견 및 수정 테스트
```

## 🚨 문제 해결

### 일반적인 문제들

#### **Cheat Engine 관련**
```
문제: "프로세스를 열 수 없습니다"
해결: 
├── 관리자 권한으로 실행
├── 안티바이러스 예외 처리
└── Windows Defender 실시간 보호 일시 해제
```

#### **x64dbg 관련**
```
문제: "디버거가 연결되지 않습니다"
해결:
├── 64비트 프로그램은 x64dbg 사용
├── 32비트 프로그램은 x32dbg 사용
└── 안티 디버그 보호가 있는 프로그램은 고급 기법 필요
```

#### **Visual Studio 관련**
```
문제: "링크 에러"
해결:
├── Windows SDK 버전 확인
├── 런타임 라이브러리 설정 확인
└── 추가 종속성 라이브러리 확인
```

### 도움 요청하기

#### **정보 수집**
```
문제 보고 시 포함할 정보:
├── 운영체제 버전 (Windows 10/11)
├── 사용 도구 및 버전
├── 에러 메시지 전문
├── 수행한 단계들
└── 스크린샷 (가능한 경우)
```

---

**🎉 축하합니다!** 

모든 도구가 정상적으로 설치되었다면 이제 [게임 일시정지](../scenario-pause-game/)로 첫 번째 실습을 시작할 준비가 완료되었습니다!