# 🛡️ 안전한 모딩 개발 가이드

**밴 방지부터 법적 고려사항까지 - 안전하고 윤리적인 모딩 실천법**

## 🚨 기본 안전 수칙

### 1. 절대 준수사항
```
❌ 절대 하지 말아야 할 것:
- 온라인 멀티플레이어에서 치팅 도구 사용
- 상업적 목적의 무단 모드 배포
- 게임 보안 시스템 악의적 우회
- 다른 플레이어의 게임 경험 방해

✅ 반드시 해야 할 것:
- 게임 백업 생성 (원본 보호)
- 오프라인 모드에서만 테스트
- 개인 학습 목적으로만 사용
- 커뮤니티 가이드라인 준수
```

### 2. 안전한 개발 환경
```bash
개발 환경 격리:
├── 가상머신 사용 (권장)
├── 별도 게임 설치 폴더
├── 네트워크 차단 (모딩 중)
└── 시스템 복원 지점 생성
```

## 🔒 안티치트 시스템 이해

### 1. EAC (Easy Anti-Cheat)
```
적용 게임:
- Elden Ring
- Dead by Daylight  
- Fortnite
- 기타 온라인 게임들

탐지 방식:
- 메모리 무결성 검사
- 프로세스 주입 탐지
- 파일 변조 확인
- 하드웨어 정보 수집
```

### 2. BattlEye
```
적용 게임:
- PUBG
- Rainbow Six Siege
- Arma 시리즈

특징:
- 커널 레벨 보호
- 실시간 메모리 스캔
- 네트워크 패킷 분석
```

### 3. VAC (Valve Anti-Cheat)
```
적용 게임:
- Counter-Strike 시리즈
- Team Fortress 2
- Left 4 Dead 시리즈

탐지 기법:
- 시그니처 기반 탐지
- 행동 패턴 분석
- 지연된 밴 시스템
```

## 🛠️ 안전한 개발 도구 사용법

### 1. Cheat Engine 안전 사용
```cpp
// 프로세스 숨김 기능 활용
void HideProcess() {
    // CE의 Stealth 모드 활성화
    // 1. Process → Process List → Hide Cheat Engine
    // 2. Settings → Debugger Options → Use VEH Debugger
    // 3. Memory View → View → Display Type → Bytes Only
}

// 안전한 스캔 옵션
Scan Options:
✅ MEM_PRIVATE only (heap/stack만)
❌ MEM_IMAGE (실행 파일 영역 제외)
❌ MEM_MAPPED (매핑된 파일 제외)
```

### 2. 메모리 패치 안전 기법
```cpp
#include <Windows.h>

class SafePatcher {
private:
    struct PatchInfo {
        uintptr_t address;
        std::vector<BYTE> original;
        bool isActive;
    };
    
    std::vector<PatchInfo> patches;
    bool isOnlineDetected = false;
    
public:
    bool SafePatch(uintptr_t addr, const std::vector<BYTE>& newBytes) {
        // 1. 온라인 상태 확인
        if (IsGameOnline()) {
            return false; // 온라인에서는 패치 거부
        }
        
        // 2. 안티치트 프로세스 확인
        if (IsAntiCheatRunning()) {
            return false;
        }
        
        // 3. 메모리 보호 상태 확인
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi));
        
        if (mbi.Protect & PAGE_GUARD) {
            return false; // 보호된 메모리
        }
        
        // 4. 안전한 패치 실행
        return ApplyPatch(addr, newBytes);
    }
    
    void RestoreOnExit() {
        // 프로그램 종료 시 모든 패치 복원
        for (auto& patch : patches) {
            if (patch.isActive) {
                WriteMemory(patch.address, patch.original);
            }
        }
    }
};
```

### 3. 안티치트 회피 기법 (교육 목적)
```cpp
// 프로세스 탐지 회피
bool IsDebuggerPresent() {
    // 일반적인 탐지 우회 (학습용)
    return false; // 항상 false 반환으로 우회
}

// 메모리 무결성 검사 대응
void MemoryProtection() {
    // 중요: 실제 사용 시 법적 문제 발생 가능
    // 교육 목적으로만 이해
    
    // 1. 메모리 페이지 권한 최소화
    // 2. 임시 패치 → 즉시 복원
    // 3. 코드 영역 분산 배치
}
```

## 📋 게임별 안전 가이드라인

### FromSoftware 게임 (Elden Ring, Dark Souls)
```bash
안전 사항:
✅ Steam 오프라인 모드 설정
✅ EAC 비활성화 후 실행
✅ Mod Engine 2 사용 (원본 보호)
✅ regulation.bin 백업

위험 요소:
❌ 온라인 상태에서 모드 사용
❌ 세이브 파일 조작
❌ PvP 관련 수치 변경
❌ 치트 아이템 획득 후 온라인 접속
```

### 언리얼 엔진 게임
```bash
PAK 모딩 안전법:
1. ~mods 폴더 사용 (비침습적)
2. Engine.ini 수정 (원본 유지)
3. 게임 무결성 검사 비활성화
4. 온라인 기능이 있다면 오프라인 모드

주의사항:
- 일부 게임은 PAK 변조 탐지
- 스팀 워크샵과 충돌 가능성
- 게임 업데이트 시 모드 무효화
```

### Unity/유니티 게임 (BepInEx)
```bash
BepInEx 모딩:
✅ doorstop_config.ini 설정
✅ plugins 폴더 격리
✅ 로그 파일 정기 삭제
✅ 개발자 모드에서만 테스트

보안 고려사항:
- BepInEx 자체가 탐지될 수 있음
- DLL 인젝션으로 인식 가능
- 게임별 정책 확인 필요
```

## 🔍 탐지 회피 전략

### 1. 시간 지연 기법
```cpp
void DelayedPatch() {
    // 게임 로딩 완료 후 패치 적용
    Sleep(30000); // 30초 대기
    
    // 패치 적용
    ApplyModifications();
    
    // 즉시 복원 (선택적)
    Sleep(1000);
    RestoreOriginal();
}
```

### 2. 메모리 분산 배치
```cpp
void ScatteredPatching() {
    // 여러 작은 패치로 분산
    std::vector<PatchInfo> smallPatches = {
        {0x140001000, {0x90}},           // 1바이트
        {0x140002000, {0x90, 0x90}},     // 2바이트  
        {0x140003000, {0x90, 0x90, 0x90}} // 3바이트
    };
    
    // 시간차 적용
    for (const auto& patch : smallPatches) {
        ApplyPatch(patch);
        Sleep(100); // 간격 두기
    }
}
```

### 3. 동적 주소 사용
```cpp
class DynamicPatcher {
public:
    void ApplyDynamicPatch() {
        // 매번 다른 방식으로 주소 계산
        uintptr_t baseAddr = GetModuleBaseAddress();
        uintptr_t offset = CalculateOffset(); // 동적 계산
        
        uintptr_t targetAddr = baseAddr + offset;
        ApplyPatch(targetAddr);
    }
    
private:
    uintptr_t CalculateOffset() {
        // AOB 스캔으로 동적 주소 찾기
        return AobScan("48 89 5C 24 ? 57 48 83 EC 20");
    }
};
```

## 🚫 밴 방지 체크리스트

### 출시 전 점검사항
```bash
[ ] 온라인 모드에서 테스트하지 않았는가?
[ ] 게임 파일을 직접 수정하지 않았는가?
[ ] 안티치트 프로세스가 실행 중이지 않은가?
[ ] 백업을 생성했는가?
[ ] 모드 제거 후 정상 실행되는가?
[ ] 세이브 파일이 손상되지 않았는가?
[ ] 게임 무결성 검사를 통과하는가?
```

### 위험도 평가
```
🟢 LOW RISK (안전):
- 오프라인 싱글플레이어 게임
- 파일 교체 없는 오버레이
- 공식 모딩 지원 게임

🟡 MEDIUM RISK (주의):
- 온라인 기능이 있는 게임의 오프라인 모드
- 메모리 패치 기반 모드
- 세이브 파일 수정

🔴 HIGH RISK (위험):
- 온라인 멀티플레이어 게임
- EAC/BattlEye 적용 게임
- 경쟁 게임 (랭크/레더)
- 상업적 이익 추구
```

## 📚 법적 및 윤리적 고려사항

### 1. 저작권 및 라이선스
```
게임 EULA 확인사항:
├── 모딩 허용 여부
├── 리버스 엔지니어링 정책
├── 파일 수정 제한사항
└── 재배포 금지 조항

일반적 제한사항:
- 게임 파일 재배포 금지
- 상업적 이용 금지  
- DRM 우회 금지
- 온라인 서비스 악용 금지
```

### 2. 윤리적 모딩 원칙
```
✅ 권장사항:
- 게임 경험 향상
- 접근성 개선
- 창작 활동 지원
- 커뮤니티 기여

❌ 금지사항:
- 다른 플레이어 방해
- 불공정한 이득 추구
- 게임 경제 파괴
- 유해 콘텐츠 제작
```

### 3. 배포 시 주의사항
```bash
모드 배포 가이드라인:
1. 원본 게임 파일 포함 금지
2. 설치/제거 방법 명시
3. 호환성 및 위험성 경고
4. 오픈소스 라이선스 고려
5. 업데이트 대응 계획
```

## 🔧 안전 도구 및 유틸리티

### 1. 백업 자동화 도구
```batch
:: 게임 백업 스크립트
@echo off
set GAME_PATH="C:\Program Files\EldenRing"
set BACKUP_PATH="D:\GameBackups\EldenRing_%date%"

echo Creating backup...
xcopy %GAME_PATH% %BACKUP_PATH% /E /I /H /Y

echo Backup completed: %BACKUP_PATH%
pause
```

### 2. 프로세스 모니터링
```cpp
#include <tlhelp32.h>

bool IsAntiCheatRunning() {
    const std::vector<std::string> antiCheatList = {
        "EasyAntiCheat.exe",
        "BEService.exe", 
        "vgtray.exe",
        "FairFight.exe"
    };
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe32)) {
        do {
            std::string processName = pe32.szExeFile;
            for (const auto& antiCheat : antiCheatList) {
                if (processName.find(antiCheat) != std::string::npos) {
                    CloseHandle(snapshot);
                    return true;
                }
            }
        } while (Process32Next(snapshot, &pe32));
    }
    
    CloseHandle(snapshot);
    return false;
}
```

### 3. 네트워크 상태 확인
```cpp
bool IsGameOnline() {
    // 네트워크 연결 상태 확인
    DWORD flags;
    if (InternetGetConnectedState(&flags, 0)) {
        // 게임별 온라인 상태 확인 로직
        return CheckGameOnlineStatus();
    }
    return false;
}
```

## 🚨 사고 대응 방안

### 1. 밴 당했을 때
```bash
즉시 대응:
1. 모든 모드 제거
2. 게임 무결성 검사 실행
3. 세이브 파일 백업에서 복원
4. Steam/플랫폼 지원팀 문의

장기 대응:
- 새 계정 생성 고려
- 하드웨어 ID 변경 (극단적)
- 모딩 방법 재검토
```

### 2. 기술적 문제 해결
```cpp
// 크래시 방지 예외 처리
__try {
    ApplyPatch(address, newBytes);
} 
__except(EXCEPTION_EXECUTE_HANDLER) {
    // 패치 실패 시 원본 복원
    RestoreOriginalBytes(address);
    LogError("Patch failed, restored original");
}
```

## 📖 추가 학습 자료

### 권장 도서
- "The Rootkit Arsenal" - Bill Blunden
- "Practical Malware Analysis" - Michael Sikorski
- "Gray Hat Hacking" - Shon Harris

### 온라인 리소스
- **Unknown Cheats**: 기술적 토론 (교육 목적)
- **MPGH**: 모딩 커뮤니티
- **Reddit /r/GameHacking**: 합법적 게임 해킹

### 법적 자문
- 각국 저작권법 숙지
- DMCA 가이드라인 이해
- 게임별 ToS 검토

---

**⚠️ 중요 면책조항**: 
이 가이드는 순수한 교육 목적으로 제작되었습니다. 모든 모딩 활동은 관련 법률과 게임 이용약관을 준수하여 개인 책임 하에 수행해야 합니다. 불법적인 용도로 사용하여 발생하는 모든 문제에 대해 작성자는 책임지지 않습니다.

**🛡️ 핵심 원칙**: 
항상 **안전하고 윤리적인** 모딩을 실천하세요!