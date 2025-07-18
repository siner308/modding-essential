#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <codecvt>
#include <locale>

/**
 * Exercise 2: 코드 케이브 인젝션 시스템
 * 
 * 목표: 실행 파일에서 사용되지 않는 공간(Code Cave)을 찾아 커스텀 코드를 삽입
 * 
 * 구현 내용:
 * 1. PE 파일에서 코드 케이브 자동 탐지
 * 2. 프로세스 메모리에서 실행 가능한 빈 공간 찾기
 * 3. 커스텀 쉘코드 생성 및 주입
 * 4. 트램폴린 후킹을 통한 원본 함수 리다이렉션
 * 5. 안전한 코드 복원 시스템
 */

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

class CodeCaveInjector {
private:
    struct CodeCave {
        uintptr_t address;
        size_t size;
        bool isExecutable;
        std::string sectionName;
    };

    struct InjectionInfo {
        uintptr_t caveAddress;
        uintptr_t hookAddress;
        std::vector<BYTE> originalBytes;
        std::vector<BYTE> shellcode;
        bool isActive;
    };

    static std::vector<CodeCave> detectedCaves;
    static std::vector<InjectionInfo> activeInjections;

public:
    // 1. PE 파일에서 코드 케이브 탐지
    static std::vector<CodeCave> FindCodeCavesInPE(const std::string& filePath, size_t minSize = 32) {
        std::wcout << L"[+] PE 파일에서 코드 케이브 탐지: " << StringToWString(filePath) << std::endl;
        
        std::vector<CodeCave> caves;
        std::ifstream file(filePath, std::ios::binary);
        
        if (!file.is_open()) {
            std::wcout << L"[-] 파일 열기 실패: " << StringToWString(filePath) << std::endl;
            return caves;
        }

        // PE 헤더 읽기
        IMAGE_DOS_HEADER dosHeader;
        file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
        
        if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            std::wcout << L"[-] 유효하지 않은 PE 파일" << std::endl;
            return caves;
        }

        file.seekg(dosHeader.e_lfanew);
        IMAGE_NT_HEADERS ntHeaders;
        file.read(reinterpret_cast<char*>(&ntHeaders), sizeof(ntHeaders));

        if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
            std::wcout << L"[-] 유효하지 않은 NT 헤더" << std::endl;
            return caves;
        }

        // 섹션 헤더들 읽기
        std::vector<IMAGE_SECTION_HEADER> sections(ntHeaders.FileHeader.NumberOfSections);
        file.read(reinterpret_cast<char*>(sections.data()), 
                 sizeof(IMAGE_SECTION_HEADER) * ntHeaders.FileHeader.NumberOfSections);

        // 각 섹션에서 코드 케이브 찾기
        for (const auto& section : sections) {
            if (section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE)) {
                auto sectionCaves = ScanSectionForCaves(file, section, minSize);
                caves.insert(caves.end(), sectionCaves.begin(), sectionCaves.end());
            }
        }

        file.close();
        
        std::wcout << L"[+] 발견된 코드 케이브: " << caves.size() << L"개" << std::endl;
        return caves;
    }

    // 2. 메모리에서 코드 케이브 탐지
    static std::vector<CodeCave> FindCodeCavesInMemory(HANDLE hProcess, size_t minSize = 32) {
        std::wcout << L"[+] 프로세스 메모리에서 코드 케이브 탐지" << std::endl;
        
        std::vector<CodeCave> caves;
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        uintptr_t address = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

        while (address < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            // 실행 가능한 커밋된 메모리 영역만 검사
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
                
                auto regionCaves = ScanMemoryRegionForCaves(hProcess, address, mbi.RegionSize, minSize);
                caves.insert(caves.end(), regionCaves.begin(), regionCaves.end());
            }

            address += mbi.RegionSize;
        }

        std::wcout << L"[+] 메모리에서 발견된 코드 케이브: " << caves.size() << L"개" << std::endl;
        return caves;
    }

    // 3. 커스텀 쉘코드 생성
    static std::vector<BYTE> CreateCustomShellcode(const std::string& message) {
        std::wcout << L"[+] 커스텀 쉘코드 생성: " << StringToWString(message) << std::endl;
        
        // 간단한 MessageBox 쉘코드 (x64)
        std::vector<BYTE> shellcode = {
            // sub rsp, 0x28 (스택 공간 확보)
            0x48, 0x83, 0xEC, 0x28,
            
            // mov rcx, 0 (hWnd = NULL)
            0x48, 0x31, 0xC9,
            
            // lea rdx, [rip + message_offset] (lpText)
            0x48, 0x8D, 0x15, 0x1A, 0x00, 0x00, 0x00,
            
            // lea r8, [rip + title_offset] (lpCaption)
            0x4C, 0x8D, 0x05, 0x1F, 0x00, 0x00, 0x00,
            
            // mov rax, MessageBoxA 주소 (런타임에 계산 필요)
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            
            // call rax
            0xFF, 0xD0,
            
            // add rsp, 0x28 (스택 복원)
            0x48, 0x83, 0xC4, 0x28,
            
            // ret
            0xC3
        };

        // 메시지 문자열 추가
        const char* msgText = message.c_str();
        shellcode.insert(shellcode.end(), msgText, msgText + message.length() + 1);
        
        // 타이틀 문자열 추가
        const char* titleText = "Code Cave Injection";
        shellcode.insert(shellcode.end(), titleText, titleText + strlen(titleText) + 1);

        return shellcode;
    }

    // 4. 코드 케이브에 쉘코드 주입
    static bool InjectShellcodeIntoCave(HANDLE hProcess, const CodeCave& cave, 
                                       const std::vector<BYTE>& shellcode) {
        std::wcout << L"[+] 코드 케이브에 쉘코드 주입 중..." << std::endl;
        std::wcout << L"    주소: 0x" << std::hex << cave.address << std::endl;
        std::wcout << L"    크기: " << std::dec << shellcode.size() << L" 바이트" << std::endl;

        if (shellcode.size() > cave.size) {
            std::wcout << L"[-] 쉘코드가 케이브보다 큼" << std::endl;
            return false;
        }

        // 메모리 보호 속성 변경
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                             shellcode.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] 메모리 보호 변경 실패: " << GetLastError() << std::endl;
            return false;
        }

        // 쉘코드 주입
        SIZE_T bytesWritten;
        if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                               shellcode.data(), shellcode.size(), &bytesWritten)) {
            std::wcout << L"[-] 메모리 쓰기 실패: " << GetLastError() << std::endl;
            return false;
        }

        // 메모리 보호 복원
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                        shellcode.size(), oldProtect, &oldProtect);

        std::wcout << L"[+] 쉘코드 주입 완료 (" << bytesWritten << L" 바이트)" << std::endl;
        return true;
    }

    // 5. 트램폴린 후킹 설치
    static bool InstallTrampolineHook(HANDLE hProcess, uintptr_t targetFunction, 
                                     uintptr_t caveFunction) {
        std::wcout << L"[+] 트램폴린 후킹 설치 중..." << std::endl;
        std::wcout << L"    대상: 0x" << std::hex << targetFunction << std::endl;
        std::wcout << L"    케이브: 0x" << std::hex << caveFunction << std::endl;

        // 원본 바이트 백업
        std::vector<BYTE> originalBytes(14); // JMP 명령어를 위한 최대 크기
        SIZE_T bytesRead;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(targetFunction), 
                              originalBytes.data(), originalBytes.size(), &bytesRead)) {
            std::wcout << L"[-] 원본 코드 읽기 실패" << std::endl;
            return false;
        }

        // 트램폴린 코드 생성 (64비트 절대 점프)
        std::vector<BYTE> trampoline = {
            0x48, 0xB8,                                    // MOV RAX, imm64
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 주소 (8바이트)
            0xFF, 0xE0,                                    // JMP RAX
            0x90, 0x90                                     // NOP padding
        };

        // 케이브 주소를 트램폴린에 삽입
        memcpy(&trampoline[2], &caveFunction, sizeof(uintptr_t));

        // 메모리 보호 해제
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(targetFunction), 
                             trampoline.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        // 트램폴린 설치
        SIZE_T bytesWritten;
        if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(targetFunction), 
                               trampoline.data(), trampoline.size(), &bytesWritten)) {
            std::wcout << L"[-] 트램폴린 쓰기 실패" << std::endl;
            return false;
        }

        // 메모리 보호 복원
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(targetFunction), 
                        trampoline.size(), oldProtect, &oldProtect);

        // 인젝션 정보 저장
        InjectionInfo injection;
        injection.caveAddress = caveFunction;
        injection.hookAddress = targetFunction;
        injection.originalBytes = originalBytes;
        injection.shellcode = trampoline;
        injection.isActive = true;
        activeInjections.push_back(injection);

        std::wcout << L"[+] 트램폴린 후킹 설치 완료" << std::endl;
        return true;
    }

    // 6. 후킹 제거 및 복원
    static bool RemoveHook(HANDLE hProcess, uintptr_t hookAddress) {
        std::wcout << L"[+] 후킹 제거 중: 0x" << std::hex << hookAddress << std::endl;

        auto it = std::find_if(activeInjections.begin(), activeInjections.end(),
            [hookAddress](const InjectionInfo& info) {
                return info.hookAddress == hookAddress && info.isActive;
            });

        if (it == activeInjections.end()) {
            std::wcout << L"[-] 활성 후킹을 찾을 수 없음" << std::endl;
            return false;
        }

        // 메모리 보호 해제
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(hookAddress), 
                             it->originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        // 원본 코드 복원
        SIZE_T bytesWritten;
        if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(hookAddress), 
                               it->originalBytes.data(), it->originalBytes.size(), &bytesWritten)) {
            std::wcout << L"[-] 원본 코드 복원 실패" << std::endl;
            return false;
        }

        // 메모리 보호 복원
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(hookAddress), 
                        it->originalBytes.size(), oldProtect, &oldProtect);

        it->isActive = false;
        std::wcout << L"[+] 후킹 제거 완료" << std::endl;
        return true;
    }

    // 7. 코드 케이브 정보 출력
    static void PrintCodeCaves(const std::vector<CodeCave>& caves) {
        std::wcout << L"\n=== 발견된 코드 케이브 ===" << std::endl;
        std::wcout << std::left << std::setw(16) << L"주소" 
                  << std::setw(8) << L"크기" 
                  << std::setw(12) << L"실행가능" 
                  << L"섹션" << std::endl;
        std::wcout << StringToWString(std::string(50, '-')) << std::endl;

        for (const auto& cave : caves) {
            std::wcout << L"0x" << std::hex << std::setw(14) << cave.address 
                      << std::dec << std::setw(8) << cave.size
                      << std::setw(12) << (cave.isExecutable ? L"Yes" : L"No")
                      << StringToWString(cave.sectionName) << std::endl;
        }
    }

private:
    // PE 섹션에서 코드 케이브 스캔
    static std::vector<CodeCave> ScanSectionForCaves(std::ifstream& file, 
                                                     const IMAGE_SECTION_HEADER& section, 
                                                     size_t minSize) {
        std::vector<CodeCave> caves;
        
        // 섹션 데이터 읽기
        std::vector<BYTE> sectionData(section.SizeOfRawData);
        file.seekg(section.PointerToRawData);
        file.read(reinterpret_cast<char*>(sectionData.data()), section.SizeOfRawData);

        // NULL 바이트 연속 영역 찾기
        size_t caveStart = 0;
        size_t caveSize = 0;
        bool inCave = false;

        for (size_t i = 0; i < sectionData.size(); ++i) {
            if (sectionData[i] == 0x00) {
                if (!inCave) {
                    caveStart = i;
                    caveSize = 1;
                    inCave = true;
                } else {
                    caveSize++;
                }
            } else {
                if (inCave && caveSize >= minSize) {
                    CodeCave cave;
                    cave.address = section.VirtualAddress + caveStart;
                    cave.size = caveSize;
                    cave.isExecutable = (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
                    cave.sectionName = std::string(reinterpret_cast<const char*>(section.Name), 8);
                    caves.push_back(cave);
                }
                inCave = false;
                caveSize = 0;
            }
        }

        // 마지막 케이브 처리
        if (inCave && caveSize >= minSize) {
            CodeCave cave;
            cave.address = section.VirtualAddress + caveStart;
            cave.size = caveSize;
            cave.isExecutable = (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            cave.sectionName = std::string(reinterpret_cast<const char*>(section.Name), 8);
            caves.push_back(cave);
        }

        return caves;
    }

    // 메모리 영역에서 코드 케이브 스캔
    static std::vector<CodeCave> ScanMemoryRegionForCaves(HANDLE hProcess, uintptr_t baseAddress, 
                                                          size_t regionSize, size_t minSize) {
        std::vector<CodeCave> caves;
        std::vector<BYTE> buffer(regionSize);
        SIZE_T bytesRead;

        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(baseAddress), 
                              buffer.data(), regionSize, &bytesRead)) {
            return caves;
        }

        // NULL 바이트 패턴 찾기
        size_t caveStart = 0;
        size_t caveSize = 0;
        bool inCave = false;

        for (size_t i = 0; i < bytesRead; ++i) {
            if (buffer[i] == 0x00 || buffer[i] == 0xCC) { // NULL 또는 INT3
                if (!inCave) {
                    caveStart = i;
                    caveSize = 1;
                    inCave = true;
                } else {
                    caveSize++;
                }
            } else {
                if (inCave && caveSize >= minSize) {
                    CodeCave cave;
                    cave.address = baseAddress + caveStart;
                    cave.size = caveSize;
                    cave.isExecutable = true;
                    cave.sectionName = "Runtime";
                    caves.push_back(cave);
                }
                inCave = false;
                caveSize = 0;
            }
        }

        return caves;
    }
};

// Static 멤버 정의
std::vector<CodeCaveInjector::CodeCave> CodeCaveInjector::detectedCaves;
std::vector<CodeCaveInjector::InjectionInfo> CodeCaveInjector::activeInjections;

// 테스트 및 데모 함수
void DemonstrateCodeCaveInjection() {
    std::wcout << L"=== 코드 케이브 인젝션 데모 ===" << std::endl;
    
    // 현재 프로세스에서 코드 케이브 찾기
    HANDLE hProcess = GetCurrentProcess();
    auto caves = CodeCaveInjector::FindCodeCavesInMemory(hProcess, 64);
    
    if (caves.empty()) {
        std::wcout << L"[-] 사용 가능한 코드 케이브가 없습니다." << std::endl;
        return;
    }

    // 코드 케이브 정보 출력
    CodeCaveInjector::PrintCodeCaves(caves);
    
    // 첫 번째 케이브에 테스트 쉘코드 주입
    auto shellcode = CodeCaveInjector::CreateCustomShellcode("Hello from Code Cave!");
    
    if (CodeCaveInjector::InjectShellcodeIntoCave(hProcess, caves[0], shellcode)) {
        std::wcout << L"[+] 코드 케이브 인젝션 성공!" << std::endl;
        std::wcout << L"주입된 주소: 0x" << std::hex << caves[0].address << std::endl;
    }
}

int main() {
    std::wcout << L"고급 코드 케이브 인젝션 시스템 v1.0" << std::endl;
    std::wcout << L"교육 및 연구 목적으로만 사용하세요." << std::endl;
    std::wcout << L"===========================================" << std::endl;

    DemonstrateCodeCaveInjection();

    std::wcout << L"\n계속하려면 Enter를 누르세요..." << std::endl;
    std::wcin.get();

    return 0;
}

/*
 * 컴파일 방법:
 * cl /EHsc exercise2_code_cave_injection.cpp
 * 
 * 실행 결과:
 * - 현재 프로세스 메모리에서 코드 케이브 탐지
 * - 발견된 케이브 목록 출력
 * - 테스트 쉘코드 주입 시연
 * 
 * 학습 포인트:
 * - PE 파일 구조 분석
 * - 메모리 스캐닝 기법
 * - 런타임 코드 주입
 * - 트램폴린 후킹
 * - 안전한 메모리 조작
 * 
 * 주의사항:
 * - 실제 환경에서는 ASLR, DEP 등 보안 기능 고려 필요
 * - 안티바이러스가 탐지할 수 있음
 * - 교육 목적으로만 사용할 것
 */