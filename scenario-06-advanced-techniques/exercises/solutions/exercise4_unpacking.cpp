#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <map>
#include <cmath>
#include <TlHelp32.h>
#include <codecvt>
#include <locale>

/**
 * Exercise 4: 패킹된 실행 파일 언패킹 시스템
 * 
 * 목표: UPX 등으로 패킹된 실행 파일을 메모리에서 덤프하여 분석
 * 
 * 구현 내용:
 * 1. 패킹 탐지 (시그니처, 엔트로피, 섹션 분석)
 * 2. 메모리 덤프 및 OEP 탐지
 * 3. Import Table 복구
 * 4. 언패킹된 파일 재구성
 * 5. 다양한 패커 지원 (UPX, ASPack, PECompact 등)
 */

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

class Unpacker {
private:
    enum PackerType {
        PACKER_NONE = 0,
        PACKER_UPX,
        PACKER_ASPACK,
        PACKER_PECOMPACT,
        PACKER_THEMIDA,
        PACKER_UNKNOWN
    };

    struct PackerSignature {
        PackerType type;
        std::string name;
        std::vector<BYTE> signature;
        size_t offset;
    };

    struct SectionInfo {
        std::string name;
        DWORD virtualAddress;
        DWORD virtualSize;
        DWORD rawAddress;
        DWORD rawSize;
        DWORD characteristics;
        float entropy;
    };

    struct ImportEntry {
        std::string moduleName;
        std::string functionName;
        DWORD address;
        WORD ordinal;
    };

    static std::vector<PackerSignature> packerSignatures;

public:
    // 1. 패커 시그니처 초기화
    static void InitializeSignatures() {
        packerSignatures.clear();
        
        // UPX 시그니처
        packerSignatures.push_back({
            PACKER_UPX, "UPX",
            {0x55, 0x50, 0x58, 0x21}, // "UPX!"
            0x00
        });
        
        // ASPack 시그니처
        packerSignatures.push_back({
            PACKER_ASPACK, "ASPack",
            {0x60, 0xE8, 0x03, 0x00, 0x00, 0x00, 0xE9, 0xEB},
            0x00
        });
        
        // PECompact 시그니처
        packerSignatures.push_back({
            PACKER_PECOMPACT, "PECompact",
            {0xEB, 0x06, 0x68, 0x00, 0x00, 0x00, 0x00, 0xC3},
            0x00
        });
        
        // Themida 시그니처
        packerSignatures.push_back({
            PACKER_THEMIDA, "Themida",
            {0x8B, 0x85, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x85},
            0x00
        });
    }

    // 2. 패킹 탐지
    static PackerType DetectPacker(const std::string& filePath) {
        std::wcout << L"[+] 패킹 탐지 중: " << StringToWString(filePath) << std::endl;
        
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::wcout << L"[-] 파일 열기 실패" << std::endl;
            return PACKER_NONE;
        }

        // PE 헤더 검증
        IMAGE_DOS_HEADER dosHeader;
        file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
        
        if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            std::wcout << L"[-] 유효하지 않은 PE 파일" << std::endl;
            return PACKER_NONE;
        }

        file.seekg(dosHeader.e_lfanew);
        IMAGE_NT_HEADERS ntHeaders;
        file.read(reinterpret_cast<char*>(&ntHeaders), sizeof(ntHeaders));

        if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
            std::wcout << L"[-] 유효하지 않은 NT 헤더" << std::endl;
            return PACKER_NONE;
        }

        // 1. 시그니처 기반 탐지
        file.seekg(0);
        std::vector<BYTE> fileData(1024); // 처음 1KB만 읽기
        file.read(reinterpret_cast<char*>(fileData.data()), fileData.size());
        
        for (const auto& sig : packerSignatures) {
            if (SearchSignature(fileData, sig.signature)) {
                std::wcout << L"[+] " << StringToWString(sig.name) << L" 패커 탐지 (시그니처)" << std::endl;
                return sig.type;
            }
        }

        // 2. 섹션 기반 휴리스틱 탐지
        auto sections = AnalyzeSections(file, ntHeaders);
        PackerType heuristicResult = HeuristicDetection(sections);
        if (heuristicResult != PACKER_NONE) {
            return heuristicResult;
        }

        // 3. 엔트로피 기반 탐지
        if (CheckHighEntropy(sections)) {
            std::wcout << L"[+] 높은 엔트로피로 인한 패킹 추정" << std::endl;
            return PACKER_UNKNOWN;
        }

        std::wcout << L"[+] 패킹되지 않은 파일" << std::endl;
        return PACKER_NONE;
    }

    // 3. 메모리 덤프
    static bool DumpProcessMemory(DWORD processId, const std::string& outputPath) {
        std::wcout << L"[+] 프로세스 메모리 덤프 중 (PID: " << processId << L")" << std::endl;
        
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) {
            std::wcout << L"[-] 프로세스 열기 실패: " << GetLastError() << std::endl;
            return false;
        }

        // 메인 모듈 정보 획득
        HMODULE hModule;
        DWORD cbNeeded;
        if (!EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
            std::wcout << L"[-] 모듈 열거 실패" << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        MODULEINFO modInfo;
        if (!GetModuleInformation(hProcess, hModule, &modInfo, sizeof(modInfo))) {
            std::wcout << L"[-] 모듈 정보 획득 실패" << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        // 메모리 읽기
        std::vector<BYTE> memoryData(modInfo.SizeOfImage);
        SIZE_T bytesRead;
        if (!ReadProcessMemory(hProcess, modInfo.lpBaseOfDll, 
                              memoryData.data(), modInfo.SizeOfImage, &bytesRead)) {
            std::wcout << L"[-] 메모리 읽기 실패: " << GetLastError() << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        // 파일로 저장
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            std::wcout << L"[-] 출력 파일 생성 실패" << std::endl;
            CloseHandle(hProcess);
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(memoryData.data()), bytesRead);
        outFile.close();

        CloseHandle(hProcess);
        
        std::wcout << L"[+] 메모리 덤프 완료: " << StringToWString(outputPath) << L" (" << bytesRead << L" 바이트)" << std::endl;
        return true;
    }

    // 4. OEP (Original Entry Point) 탐지
    static DWORD FindOEP(const std::vector<BYTE>& memoryData, DWORD baseAddress) {
        std::wcout << L"[+] OEP 탐지 중..." << std::endl;
        
        // PE 헤더 분석
        if (memoryData.size() < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) {
            return 0;
        }

        const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(memoryData.data());
        const IMAGE_NT_HEADERS* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
            memoryData.data() + dosHeader->e_lfanew);

        DWORD entryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        
        // 일반적인 프로그램 시작 패턴 찾기
        std::vector<std::vector<BYTE>> commonPatterns = {
            {0x55, 0x8B, 0xEC},                    // PUSH EBP; MOV EBP, ESP
            {0x6A, 0xFF, 0x68},                    // PUSH -1; PUSH
            {0x68, 0x00, 0x00, 0x00, 0x00},        // PUSH imm32
            {0x53, 0x56, 0x57},                    // PUSH EBX; PUSH ESI; PUSH EDI
            {0x83, 0xEC},                          // SUB ESP, imm8
            {0x48, 0x83, 0xEC}                     // SUB RSP, imm8 (x64)
        };

        // EntryPoint 주변에서 패턴 검색
        for (int offset = -2048; offset <= 2048; offset += 4) {
            DWORD checkAddress = entryPoint + offset;
            
            if (checkAddress >= memoryData.size() - 8) continue;
            
            for (const auto& pattern : commonPatterns) {
                if (MatchPattern(memoryData, checkAddress, pattern)) {
                    std::wcout << L"[+] OEP 후보 발견: 0x" << std::hex << (baseAddress + checkAddress) << std::endl;
                    
                    // 추가 검증
                    if (ValidateOEP(memoryData, checkAddress)) {
                        std::wcout << L"[+] OEP 확정: 0x" << std::hex << (baseAddress + checkAddress) << std::endl;
                        return baseAddress + checkAddress;
                    }
                }
            }
        }

        std::wcout << L"[+] 기본 EntryPoint 사용: 0x" << std::hex << (baseAddress + entryPoint) << std::endl;
        return baseAddress + entryPoint;
    }

    // 5. Import Table 복구
    static bool FixImportTable(std::vector<BYTE>& memoryData, DWORD baseAddress) {
        std::wcout << L"[+] Import Table 복구 중..." << std::endl;
        
        // IAT 스캐닝을 통한 Import 복구
        auto imports = ScanForImports(memoryData, baseAddress);
        
        if (imports.empty()) {
            std::wcout << L"[-] Import 정보를 찾을 수 없음" << std::endl;
            return false;
        }

        std::wcout << L"[+] 발견된 Import: " << imports.size() << L"개" << std::endl;
        
        // Import 정보 출력
        std::map<std::string, std::vector<ImportEntry>> moduleImports;
        for (const auto& import : imports) {
            moduleImports[import.moduleName].push_back(import);
        }

        for (const auto& module : moduleImports) {
            std::wcout << L"  " << StringToWString(module.first) << L": " << module.second.size() << L"개 함수" << std::endl;
        }

        // 실제 Import Table 재구성은 복잡하므로 여기서는 분석만 수행
        return true;
    }

    // 6. 언패킹된 파일 재구성
    static bool ReconstructPE(const std::vector<BYTE>& memoryData, 
                             const std::string& outputPath, DWORD oep) {
        std::wcout << L"[+] PE 파일 재구성 중..." << std::endl;
        
        if (memoryData.size() < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) {
            return false;
        }

        // 메모리 이미지를 파일 형태로 변환
        std::vector<BYTE> reconstructed = memoryData;
        
        // PE 헤더 수정
        IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(reconstructed.data());
        IMAGE_NT_HEADERS* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(
            reconstructed.data() + dosHeader->e_lfanew);

        // OEP 설정
        ntHeaders->OptionalHeader.AddressOfEntryPoint = oep;
        
        // 섹션 특성 정규화
        IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
            // 실행 가능한 섹션 복원
            if (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                sections[i].Characteristics |= IMAGE_SCN_CNT_CODE;
            }
            
            // RawSize와 VirtualSize 동기화
            sections[i].SizeOfRawData = sections[i].Misc.VirtualSize;
            
            // 파일 오프셋 조정 (간단화된 버전)
            sections[i].PointerToRawData = sections[i].VirtualAddress;
        }

        // 파일 저장
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            std::wcout << L"[-] 출력 파일 생성 실패" << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(reconstructed.data()), reconstructed.size());
        outFile.close();

        std::wcout << L"[+] PE 파일 재구성 완료: " << StringToWString(outputPath) << std::endl;
        return true;
    }

    // 7. 전체 언패킹 프로세스
    static bool UnpackFile(const std::string& inputPath, const std::string& outputPath) {
        std::wcout << L"=== 언패킹 프로세스 시작 ===" << std::endl;
        std::wcout << L"입력 파일: " << StringToWString(inputPath) << std::endl;
        std::wcout << L"출력 파일: " << StringToWString(outputPath) << std::endl;
        
        // 1. 패킹 탐지
        PackerType packerType = DetectPacker(inputPath);
        if (packerType == PACKER_NONE) {
            std::wcout << L"[!] 패킹되지 않은 파일입니다." << std::endl;
            return false;
        }

        // 2. 프로세스 실행
        std::wcout << L"[+] 언패킹을 위해 프로세스 실행 중..." << std::endl;
        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);

        if (!CreateProcessA(inputPath.c_str(), nullptr, nullptr, nullptr, 
                           FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
            std::wcout << L"[-] 프로세스 생성 실패: " << GetLastError() << std::endl;
            return false;
        }

        // 3. 언패킹 대기 (실제로는 더 정교한 OEP 탐지 필요)
        std::wcout << L"[+] 언패킹 대기 중..." << std::endl;
        ResumeThread(pi.hThread);
        Sleep(2000); // 언패킹 시간 대기
        SuspendThread(pi.hThread);

        // 4. 메모리 덤프
        std::string tempDumpPath = outputPath + ".dump";
        if (!DumpProcessMemory(pi.dwProcessId, tempDumpPath)) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return false;
        }

        // 5. 덤프 파일 읽기
        std::ifstream dumpFile(tempDumpPath, std::ios::binary);
        std::vector<BYTE> memoryData((std::istreambuf_iterator<char>(dumpFile)),
                                    std::istreambuf_iterator<char>());
        dumpFile.close();

        // 6. OEP 탐지
        DWORD oep = FindOEP(memoryData, 0x400000); // 기본 ImageBase

        // 7. Import Table 복구
        FixImportTable(memoryData, 0x400000);

        // 8. PE 파일 재구성
        bool success = ReconstructPE(memoryData, outputPath, oep);

        // 9. 정리
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        DeleteFileA(tempDumpPath.c_str());

        if (success) {
            std::wcout << L"[+] 언패킹 완료!" << std::endl;
        } else {
            std::wcout << L"[-] 언패킹 실패!" << std::endl;
        }

        return success;
    }

private:
    // 헬퍼 함수들
    static bool SearchSignature(const std::vector<BYTE>& data, const std::vector<BYTE>& signature) {
        if (signature.empty() || data.size() < signature.size()) {
            return false;
        }

        for (size_t i = 0; i <= data.size() - signature.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < signature.size(); ++j) {
                if (data[i + j] != signature[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        return false;
    }

    static std::vector<SectionInfo> AnalyzeSections(std::ifstream& file, const IMAGE_NT_HEADERS& ntHeaders) {
        std::vector<SectionInfo> sections;
        
        IMAGE_SECTION_HEADER sectionHeader;
        for (int i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++) {
            file.read(reinterpret_cast<char*>(&sectionHeader), sizeof(sectionHeader));
            
            SectionInfo info;
            info.name = std::string(reinterpret_cast<const char*>(sectionHeader.Name), 8);
            info.virtualAddress = sectionHeader.VirtualAddress;
            info.virtualSize = sectionHeader.Misc.VirtualSize;
            info.rawAddress = sectionHeader.PointerToRawData;
            info.rawSize = sectionHeader.SizeOfRawData;
            info.characteristics = sectionHeader.Characteristics;
            
            // 엔트로피 계산
            std::vector<BYTE> sectionData(sectionHeader.SizeOfRawData);
            auto currentPos = file.tellg();
            file.seekg(sectionHeader.PointerToRawData);
            file.read(reinterpret_cast<char*>(sectionData.data()), sectionHeader.SizeOfRawData);
            file.seekg(currentPos);
            
            info.entropy = CalculateEntropy(sectionData);
            sections.push_back(info);
        }
        
        return sections;
    }

    static PackerType HeuristicDetection(const std::vector<SectionInfo>& sections) {
        // UPX 특성 확인
        for (const auto& section : sections) {
            if (section.name.find("UPX") != std::string::npos) {
                std::wcout << L"[+] UPX 패커 탐지 (섹션명)" << std::endl;
                return PACKER_UPX;
            }
            if (section.name == ".aspack" || section.name == ".adata") {
                std::wcout << L"[+] ASPack 패커 탐지 (섹션명)" << std::endl;
                return PACKER_ASPACK;
            }
        }
        
        return PACKER_NONE;
    }

    static bool CheckHighEntropy(const std::vector<SectionInfo>& sections) {
        for (const auto& section : sections) {
            if (section.entropy > 7.5f && (section.characteristics & IMAGE_SCN_MEM_EXECUTE)) {
                std::wcout << L"[+] 높은 엔트로피 실행 섹션 발견: " << StringToWString(section.name) 
                         << L" (엔트로피: " << section.entropy << L")" << std::endl;
                return true;
            }
        }
        return false;
    }

    static float CalculateEntropy(const std::vector<BYTE>& data) {
        if (data.empty()) return 0.0f;
        
        int frequency[256] = {0};
        for (BYTE b : data) {
            frequency[b]++;
        }
        
        float entropy = 0.0f;
        for (int i = 0; i < 256; i++) {
            if (frequency[i] > 0) {
                float probability = static_cast<float>(frequency[i]) / data.size();
                entropy -= probability * std::log2(probability);
            }
        }
        
        return entropy;
    }

    static bool MatchPattern(const std::vector<BYTE>& data, DWORD offset, const std::vector<BYTE>& pattern) {
        if (offset + pattern.size() > data.size()) {
            return false;
        }
        
        for (size_t i = 0; i < pattern.size(); i++) {
            if (data[offset + i] != pattern[i]) {
                return false;
            }
        }
        return true;
    }

    static bool ValidateOEP(const std::vector<BYTE>& data, DWORD offset) {
        // OEP 검증: 유효한 명령어들이 연속으로 나타나는지 확인
        // 간단한 검증만 수행
        if (offset + 16 > data.size()) return false;
        
        // NULL 바이트가 연속으로 나타나면 유효하지 않음
        int nullCount = 0;
        for (int i = 0; i < 16; i++) {
            if (data[offset + i] == 0x00) {
                nullCount++;
                if (nullCount > 4) return false;
            }
            else {
                nullCount = 0;
            }
        }
        
        return true;
    }

    static std::vector<ImportEntry> ScanForImports(const std::vector<BYTE>& data, DWORD baseAddress) {
        std::vector<ImportEntry> imports;
        
        // 간단한 IAT 스캐닝 (실제로는 더 복잡한 구현 필요)
        // 여기서는 일반적인 Windows API 이름들을 찾는 방식으로 구현
        std::vector<std::string> commonAPIs = {
            "GetProcAddress", "LoadLibraryA", "GetModuleHandleA", 
            "VirtualAlloc", "VirtualProtect", "CreateFileA",
            "ReadFile", "WriteFile", "CloseHandle"
        };
        
        for (const auto& api : commonAPIs) {
            // 문자열 검색 (실제로는 IAT 구조 분석 필요)
            for (size_t i = 0; i <= data.size() - api.length(); i++) {
                if (memcmp(data.data() + i, api.c_str(), api.length()) == 0) {
                    ImportEntry entry;
                    entry.functionName = api;
                    entry.moduleName = "kernel32.dll"; // 간단화
                    entry.address = baseAddress + static_cast<DWORD>(i);
                    imports.push_back(entry);
                    break;
                }
            }
        }
        
        return imports;
    }
};

// Static 멤버 정의
std::vector<Unpacker::PackerSignature> Unpacker::packerSignatures;

int main(int argc, char* argv[]) {
    std::wcout << L"고급 실행 파일 언패킹 시스템 v1.0" << std::endl;
    std::wcout << L"교육 및 연구 목적으로만 사용하세요." << std::endl;
    std::wcout << L"=====================================" << std::endl;

    if (argc != 3) {
        std::wcout << L"사용법: " << StringToWString(argv[0]) << L" <입력파일> <출력파일>" << std::endl;
        std::wcout << L"예제: " << StringToWString(argv[0]) << L" packed.exe unpacked.exe" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    // 패커 시그니처 초기화
    Unpacker::InitializeSignatures();

    // 언패킹 실행
    if (Unpacker::UnpackFile(inputPath, outputPath)) {
        std::wcout << L"\n✅ 언패킹 성공!" << std::endl;
        std::wcout << L"언패킹된 파일: " << StringToWString(outputPath) << std::endl;
    } else {
        std::wcout << L"\n❌ 언패킹 실패!" << std::endl;
    }

    std::wcout << L"\n계속하려면 Enter를 누르세요..." << std::endl;
    std::wcin.get();

    return 0;
}

/*
 * 컴파일 방법:
 * cl /EHsc exercise4_unpacking.cpp
 * 
 * 사용 방법:
 * exercise4_unpacking.exe packed_file.exe unpacked_file.exe
 * 
 * 지원 패커:
 * - UPX (Ultimate Packer for eXecutables)
 * - ASPack
 * - PECompact
 * - Themida (부분적)
 * - 기타 일반적인 패커들
 * 
 * 언패킹 과정:
 * 1. 패킹 탐지 (시그니처 + 휴리스틱)
 * 2. 프로세스 실행 및 메모리 덤프
 * 3. OEP (Original Entry Point) 탐지
 * 4. Import Table 복구
 * 5. PE 파일 재구성
 * 
 * 학습 포인트:
 * - PE 파일 구조 분석
 * - 패커 탐지 기법
 * - 메모리 덤프 기술
 * - OEP 찾기 알고리즘
 * - Import Table 복구
 * - 바이너리 재구성
 * 
 * 제한사항:
 * - 고급 패커 (Themida, VMProtect 등)는 완전 지원 안됨
 * - Anti-dump, Anti-debug 우회 기능 없음
 * - 실제 프로덕션 환경에서는 더 정교한 구현 필요
 */