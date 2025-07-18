/*
 * Exercise 5: 프리셋 시스템
 * 
 * 문제: 다양한 FPS 프리셋을 저장하고 불러오는 시스템을 구현하세요.
 * 
 * 학습 목표:
 * - 설정 데이터 관리
 * - JSON/XML 파일 처리
 * - 사용자 인터페이스 구현
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <json/json.h>
#include <iomanip>
#include <chrono>
#include <limits> // For std::numeric_limits

#pragma comment(lib, "jsoncpp.lib")

class FPSPresetSystem {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct FPSPreset {
        std::string name;
        std::string description;
        float targetFPS;
        std::string category;
        std::vector<std::string> tags;
        bool requiresWarning;
        std::string warningMessage;
        int64_t createdTime;
        int64_t lastUsed;
        int useCount;
        std::map<std::string, std::string> metadata;
    };
    
    struct PresetCollection {
        std::string name;
        std::string description;
        std::vector<std::string> presetNames;
        std::string author;
        std::string version;
        int64_t createdTime;
    };
    
    struct ApplicationResult {
        bool success;
        std::string message;
        float previousFPS;
        float newFPS;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::map<std::string, FPSPreset> presets;
    std::map<std::string, PresetCollection> collections;
    std::vector<ApplicationResult> applicationHistory;
    std::string configDirectory;
    std::string presetsFile;
    std::string collectionsFile;
    std::string historyFile;

public:
    FPSPresetSystem() : processHandle(nullptr), processId(0) {
        configDirectory = "fps_presets/";
        presetsFile = configDirectory + "presets.json";
        collectionsFile = configDirectory + "collections.json";
        historyFile = configDirectory + "history.json";
        
        // 설정 디렉토리 생성
        CreateDirectoryA(configDirectory.c_str(), nullptr);
        
        LoadDefaultPresets();
    }
    
    ~FPSPresetSystem() {
        SaveAllConfigurations();
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }
    
    bool Initialize(const std::wstring& targetProcess) {
        processName = targetProcess;
        
        // 프로세스 찾기
        if (!FindProcess()) {
            return false;
        }
        
        // 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, 
                                  FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        // 저장된 설정 로드
        LoadAllConfigurations();
        
        std::wcout << L"FPS 프리셋 시스템 초기화 완료" << std::endl;
        return true;
    }
    
    void LoadDefaultPresets() {
        // 기본 프리셋들 생성
        CreatePreset("Standard_60", "표준 60 FPS", 60.0f, "Standard", 
                    {"standard", "safe"}, false, "");
        
        CreatePreset("Smooth_120", "부드러운 120 FPS", 120.0f, "High Performance", 
                    {"smooth", "high-refresh"}, false, "");
        
        CreatePreset("Gaming_144", "게이밍 144 FPS", 144.0f, "Gaming", 
                    {"gaming", "competitive"}, true, 
                    "144 FPS는 일부 게임에서 문제를 일으킬 수 있습니다.");
        
        CreatePreset("Extreme_240", "극한 240 FPS", 240.0f, "Extreme", 
                    {"extreme", "benchmark"}, true, 
                    "240 FPS는 매우 높은 설정으로 게임 안정성에 영향을 줄 수 있습니다.");
        
        CreatePreset("Cinema_30", "시네마틱 30 FPS", 30.0f, "Cinematic", 
                    {"cinematic", "story"}, false, "");
        
        CreatePreset("Battery_45", "배터리 절약 45 FPS", 45.0f, "Power Saving", 
                    {"battery", "laptop"}, false, "");
        
        CreatePreset("Unlocked", "무제한 FPS", 999.0f, "Unlimited", 
                    {"unlimited", "benchmark"}, true, 
                    "무제한 FPS는 하드웨어에 높은 부하를 가할 수 있습니다.");
        
        // 게임별 특화 프리셋
        CreatePreset("Souls_Safe", "소울즈 게임 안전 모드", 90.0f, "Game Specific", 
                    {"souls", "safe", "fromsoft"}, false, "");
        
        CreatePreset("Skyrim_Stable", "스카이림 안정 모드", 72.0f, "Game Specific", 
                    {"skyrim", "creation-engine"}, false, "");
        
        CreatePreset("Racing_165", "레이싱 게임 165 FPS", 165.0f, "Game Specific", 
                    {"racing", "competitive"}, false, "");
    }
    
    void CreatePreset(const std::string& name, const std::string& description, 
                     float targetFPS, const std::string& category,
                     const std::vector<std::string>& tags, bool requiresWarning,
                     const std::string& warningMessage) {
        
        FPSPreset preset;
        preset.name = name;
        preset.description = description;
        preset.targetFPS = targetFPS;
        preset.category = category;
        preset.tags = tags;
        preset.requiresWarning = requiresWarning;
        preset.warningMessage = warningMessage;
        preset.createdTime = GetCurrentTimestamp();
        preset.lastUsed = 0;
        preset.useCount = 0;
        
        presets[name] = preset;
    }
    
    void ShowAllPresets() {
        std::wcout << L"\n=== FPS 프리셋 목록 ===" << std::endl;
        
        // 카테고리별로 정렬
        std::map<std::string, std::vector<std::string>> categorizedPresets;
        
        for (const auto& pair : presets) {
            categorizedPresets[pair.second.category].push_back(pair.first);
        }
        
        for (const auto& categoryPair : categorizedPresets) {
            std::wcout << L"\n[" << std::wstring(categoryPair.first.begin(), categoryPair.first.end()) << L"]" << std::endl;
            
            for (const std::string& presetName : categoryPair.second) {
                const auto& preset = presets[presetName];
                
                std::wcout << L"  " << std::wstring(preset.name.begin(), preset.name.end()) 
                           << L" (" << preset.targetFPS << L" FPS)" << std::endl;
                std::wcout << L"    " << std::wstring(preset.description.begin(), preset.description.end()) << std::endl;
                
                if (!preset.tags.empty()) {
                    std::wcout << L"    태그: ";
                    for (size_t i = 0; i < preset.tags.size(); ++i) {
                        std::wcout << std::wstring(preset.tags[i].begin(), preset.tags[i].end());
                        if (i < preset.tags.size() - 1) std::wcout << L", ";
                    }
                    std::wcout << std::endl;
                }
                
                if (preset.useCount > 0) {
                    std::wcout << L"    사용 횟수: " << preset.useCount << L"회" << std::endl;
                }
                
                if (preset.requiresWarning) {
                    std::wcout << L"    ⚠️ 주의 필요" << std::endl;
                }
                
                std::wcout << std::endl;
            }
        }
    }
    
    bool ApplyPreset(const std::string& presetName, uintptr_t address) {
        auto it = presets.find(presetName);
        if (it == presets.end()) {
            std::wcout << L"프리셋을 찾을 수 없습니다: " << std::wstring(presetName.begin(), presetName.end()) << std::endl;
            return false;
        }
        
        FPSPreset& preset = it->second;
        
        // 경고 메시지 표시
        if (preset.requiresWarning && !preset.warningMessage.empty()) {
            std::wcout << L"\n⚠️ 경고: " << std::wstring(preset.warningMessage.begin(), preset.warningMessage.end()) << std::endl;
            std::wcout << L"계속하시겠습니까? (y/n): ";
            
            wchar_t response;
            std::wcin >> response;
            
            if (response != L'y' && response != L'Y') {
                std::wcout << L"프리셋 적용이 취소되었습니다." << std::endl;
                return false;
            }
        }
        
        // 현재 FPS 값 읽기
        float currentFPS;
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                              &currentFPS, sizeof(currentFPS), &bytesRead) || 
            bytesRead != sizeof(currentFPS)) {
            
            ApplicationResult result;
            result.success = false;
            result.message = "현재 FPS 값을 읽을 수 없습니다";
            result.timestamp = std::chrono::system_clock::now();
            applicationHistory.push_back(result);
            
            std::wcout << L"현재 FPS 값을 읽을 수 없습니다" << std::endl;
            return false;
        }
        
        std::wcout << L"\n프리셋 적용 중: " << std::wstring(preset.name.begin(), preset.name.end()) << std::endl;
        std::wcout << L"FPS 변경: " << currentFPS << L" -> " << preset.targetFPS << std::endl;
        
        // FPS 값 변경
        SIZE_T bytesWritten;
        bool success = WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address),
                                        &preset.targetFPS, sizeof(preset.targetFPS), &bytesWritten) 
                       && bytesWritten == sizeof(preset.targetFPS);
        
        // 결과 기록
        ApplicationResult result;
        result.success = success;
        result.previousFPS = currentFPS;
        result.newFPS = preset.targetFPS;
        result.timestamp = std::chrono::system_clock::now();
        
        if (success) {
            result.message = "프리셋 적용 성공: " + preset.name;
            preset.lastUsed = GetCurrentTimestamp();
            preset.useCount++;
            
            std::wcout << L"✓ 프리셋 적용 완료" << std::endl;
        } else {
            result.message = "프리셋 적용 실패: " + std::to_string(GetLastError());
            std::wcout << L"✗ 프리셋 적용 실패. 오류: " << GetLastError() << std::endl;
        }
        
        applicationHistory.push_back(result);
        return success;
    }
    
    void CreateCustomPreset() {
        std::wcout << L"\n=== 커스텀 프리셋 생성 ===" << std::endl;
        
        std::string name, description, category;
        float targetFPS;
        bool requiresWarning = false;
        std::string warningMessage;
        
        std::wcout << L"프리셋 이름: ";
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, name);
        
        std::wcout << L"설명: ";
        std::getline(std::wcin, description);
        
        std::wcout << L"목표 FPS: ";
        std::wcin >> targetFPS;
        
        std::wcout << L"카테고리: ";
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, category);
        
        std::wcout << L"경고 메시지가 필요합니까? (y/n): ";
        wchar_t needWarning;
        std::wcin >> needWarning;
        
        if (needWarning == L'y' || needWarning == L'Y') {
            requiresWarning = true;
            std::wcout << L"경고 메시지: ";
            std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
            std::getline(std::wcin, warningMessage);
        }
        
        // 태그 입력
        std::vector<std::string> tags;
        std::wcout << L"태그 (쉼표로 구분, 선택사항): ";
        std::wstring tagInputW;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, tagInputW);
        
        std::string tagInput(tagInputW.begin(), tagInputW.end());

        if (!tagInput.empty()) {
            std::stringstream ss(tagInput);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                // 앞뒤 공백 제거
                tag.erase(0, tag.find_first_not_of(" \t"));
                tag.erase(tag.find_last_not_of(" \t") + 1);
                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }
        }
        
        CreatePreset(name, description, targetFPS, category, tags, requiresWarning, warningMessage);
        
        std::wcout << L"\n커스텀 프리셋이 생성되었습니다: " << std::wstring(name.begin(), name.end()) << std::endl;
    }
    
    void CreatePresetCollection() {
        std::wcout << L"\n=== 프리셋 컬렉션 생성 ===" << std::endl;
        
        std::string name, description, author, version;
        
        std::wcout << L"컬렉션 이름: ";
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, name);
        
        std::wcout << L"설명: ";
        std::getline(std::wcin, description);
        
        std::wcout << L"작성자: ";
        std::getline(std::wcin, author);
        
        std::wcout << L"버전: ";
        std::getline(std::wcin, version);
        
        PresetCollection collection;
        collection.name = name;
        collection.description = description;
        collection.author = author;
        collection.version = version;
        collection.createdTime = GetCurrentTimestamp();
        
        // 포함할 프리셋 선택
        std::wcout << L"\n포함할 프리셋을 선택하세요:" << std::endl;
        
        std::vector<std::string> availablePresets;
        int index = 1;
        for (const auto& pair : presets) {
            std::wcout << L"  " << index++ << L". " << std::wstring(pair.first.begin(), pair.first.end()) << std::endl;
            availablePresets.push_back(pair.first);
        }
        
        std::wcout << L"\n선택할 프리셋 번호들 (쉼표로 구분): ";
        std::wstring selectionInputW;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, selectionInputW);
        
        std::string selectionInput(selectionInputW.begin(), selectionInputW.end());

        std::stringstream ss(selectionInput);
        std::string numStr;
        while (std::getline(ss, numStr, ',')) {
            int num = std::stoi(numStr);
            if (num >= 1 && num <= static_cast<int>(availablePresets.size())) {
                collection.presetNames.push_back(availablePresets[num - 1]);
            }
        }
        
        collections[name] = collection;
        
        std::wcout << L"\n컬렉션이 생성되었습니다: " << std::wstring(name.begin(), name.end()) << std::endl;
        std::wcout << L"포함된 프리셋 수: " << collection.presetNames.size() << std::endl;
    }
    
    void ShowCollections() {
        std::wcout << L"\n=== 프리셋 컬렉션 ===" << std::endl;
        
        if (collections.empty()) {
            std::wcout << L"생성된 컬렉션이 없습니다." << std::endl;
            return;
        }
        
        for (const auto& pair : collections) {
            const auto& collection = pair.second;
            
            std::wcout << L"\n[" << std::wstring(collection.name.begin(), collection.name.end()) << L"]" << std::endl;
            std::wcout << L"  설명: " << std::wstring(collection.description.begin(), collection.description.end()) << std::endl;
            std::wcout << L"  작성자: " << std::wstring(collection.author.begin(), collection.author.end()) << std::endl;
            std::wcout << L"  버전: " << std::wstring(collection.version.begin(), collection.version.end()) << std::endl;
            std::wcout << L"  프리셋 수: " << collection.presetNames.size() << std::endl;
            
            std::wcout << L"  포함된 프리셋:" << std::endl;
            for (const std::string& presetName : collection.presetNames) {
                auto presetIt = presets.find(presetName);
                if (presetIt != presets.end()) {
                    std::wcout << L"    - " << std::wstring(presetName.begin(), presetName.end()) 
                               << L" (" << presetIt->second.targetFPS << L" FPS)" << std::endl;
                }
            }
        }
    }
    
    void SearchPresets() {
        std::wcout << L"\n=== 프리셋 검색 ===" << std::endl;
        std::wcout << L"검색 키워드 (이름, 설명, 태그): ";
        
        std::wstring keywordW;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, keywordW);
        
        std::string keyword(keywordW.begin(), keywordW.end());
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
        
        std::wcout << L"\n검색 결과:" << std::endl;
        bool found = false;
        
        for (const auto& pair : presets) {
            const auto& preset = pair.second;
            
            // 이름 검색
            std::string lowerName = preset.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            // 설명 검색
            std::string lowerDesc = preset.description;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
            
            // 태그 검색
            bool tagMatch = false;
            for (const std::string& tag : preset.tags) {
                std::string lowerTag = tag;
                std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                if (lowerTag.find(keyword) != std::string::npos) {
                    tagMatch = true;
                    break;
                }
            }
            
            if (lowerName.find(keyword) != std::string::npos || 
                lowerDesc.find(keyword) != std::string::npos || 
                tagMatch) {
                
                std::wcout << L"  ✓ " << std::wstring(preset.name.begin(), preset.name.end()) 
                           << L" (" << preset.targetFPS << L" FPS)" << std::endl;
                std::wcout << L"    " << std::wstring(preset.description.begin(), preset.description.end()) << std::endl;
                found = true;
            }
        }
        
        if (!found) {
            std::wcout << L"검색 결과가 없습니다." << std::endl;
        }
    }
    
    void ShowApplicationHistory() {
        std::wcout << L"\n=== 적용 기록 ===" << std::endl;
        
        if (applicationHistory.empty()) {
            std::wcout << L"적용 기록이 없습니다." << std::endl;
            return;
        }
        
        // 최근 10개 기록만 표시
        size_t start = applicationHistory.size() > 10 ? applicationHistory.size() - 10 : 0;
        
        for (size_t i = start; i < applicationHistory.size(); ++i) {
            const auto& record = applicationHistory[i];
            
            auto time_t = std::chrono::system_clock::to_time_t(record.timestamp);
            std::wcout << L"  " << std::put_time(std::localtime(&time_t), L"%Y-%m-%d %H:%M:%S");
            
            if (record.success) {
                std::wcout << L" ✓ " << record.previousFPS << L" -> " << record.newFPS << L" FPS";
            } else {
                std::wcout << L" ✗ 실패";
            }
            
            std::wcout << L" (" << std::wstring(record.message.begin(), record.message.end()) << L")" << std::endl;
        }
    }
    
    void ExportPresets() {
        std::wcout << L"\n=== 프리셋 내보내기 ===" << std::endl;
        std::wcout << L"내보낼 파일명: ";
        
        std::wstring filenameW;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, filenameW);
        
        std::string filename(filenameW.begin(), filenameW.end());

        if (!filename.ends_with(".json")) {
            filename += ".json";
        }
        
        Json::Value root;
        root["metadata"]["exportTime"] = GetCurrentTimestamp();
        root["metadata"]["source"] = "FPS Preset System";
        root["metadata"]["version"] = "1.0";
        
        for (const auto& pair : presets) {
            const auto& preset = pair.second;
            Json::Value presetJson;
            
            presetJson["name"] = preset.name;
            presetJson["description"] = preset.description;
            presetJson["targetFPS"] = preset.targetFPS;
            presetJson["category"] = preset.category;
            presetJson["requiresWarning"] = preset.requiresWarning;
            presetJson["warningMessage"] = preset.warningMessage;
            presetJson["createdTime"] = preset.createdTime;
            presetJson["useCount"] = preset.useCount;
            
            for (const std::string& tag : preset.tags) {
                presetJson["tags"].append(tag);
            }
            
            root["presets"][preset.name] = presetJson;
        }
        
        std::ofstream file(filename);
        if (file.is_open()) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &file);
            file.close();
            
            std::wcout << L"프리셋이 내보내졌습니다: " << std::wstring(filename.begin(), filename.end()) << std::endl;
        } else {
            std::wcout << L"파일 생성 실패" << std::endl;
        }
    }
    
    void ImportPresets() {
        std::wcout << L"\n=== 프리셋 가져오기 ===" << std::endl;
        std::wcout << L"가져올 파일명: ";
        
        std::wstring filenameW;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); // Clear buffer
        std::getline(std::wcin, filenameW);
        
        std::string filename(filenameW.begin(), filenameW.end());

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::wcout << L"파일을 열 수 없습니다: " << std::wstring(filename.begin(), filename.end()) << std::endl;
            return;
        }
        
        Json::Value root;
        try {
            file >> root;
            file.close();
            
            if (root.isMember("presets")) {
                int importCount = 0;
                
                for (const auto& presetName : root["presets"].getMemberNames()) {
                    const Json::Value& presetJson = root["presets"][presetName];
                    
                    FPSPreset preset;
                    preset.name = presetJson["name"].asString();
                    preset.description = presetJson["description"].asString();
                    preset.targetFPS = presetJson["targetFPS"].asFloat();
                    preset.category = presetJson["category"].asString();
                    preset.requiresWarning = presetJson["requiresWarning"].asBool();
                    preset.warningMessage = presetJson["warningMessage"].asString();
                    preset.createdTime = presetJson["createdTime"].asInt64();
                    preset.useCount = presetJson["useCount"].asInt();
                    preset.lastUsed = 0;
                    
                    if (presetJson.isMember("tags")) {
                        for (const auto& tag : presetJson["tags"]) {
                            preset.tags.push_back(tag.asString());
                        }
                    }
                    
                    presets[preset.name] = preset;
                    importCount++;
                }
                
                std::wcout << L"프리셋 " << importCount << L"개가 가져와졌습니다." << std::endl;
            } else {
                std::wcout << L"유효하지 않은 프리셋 파일입니다." << std::endl;
            }
        } catch (const Json::Exception& e) {
            std::wcout << L"파일 파싱 오류: " << std::wstring(e.what(), e.what() + strlen(e.what())) << std::endl;
        }
    }
    
private:
    bool FindProcess() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        bool found = false;
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    processId = processEntry.th32ProcessID;
                    found = true;
                    break;
                }
            }
            while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
        return found;
    }
    
    int64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    void LoadAllConfigurations() {
        LoadPresets();
        LoadCollections();
        LoadHistory();
    }
    
    void SaveAllConfigurations() {
        SavePresets();
        SaveCollections();
        SaveHistory();
    }
    
    void LoadPresets() {
        std::ifstream file(presetsFile);
        if (!file.is_open()) return;
        
        Json::Value root;
        try {
            file >> root;
            file.close();
            
            for (const auto& presetName : root.getMemberNames()) {
                const Json::Value& presetJson = root[presetName];
                
                FPSPreset preset;
                preset.name = presetJson["name"].asString();
                preset.description = presetJson["description"].asString();
                preset.targetFPS = presetJson["targetFPS"].asFloat();
                preset.category = presetJson["category"].asString();
                preset.requiresWarning = presetJson["requiresWarning"].asBool();
                preset.warningMessage = presetJson["warningMessage"].asString();
                preset.createdTime = presetJson["createdTime"].asInt64();
                preset.lastUsed = presetJson["lastUsed"].asInt64();
                preset.useCount = presetJson["useCount"].asInt();
                
                if (presetJson.isMember("tags")) {
                    for (const auto& tag : presetJson["tags"]) {
                        preset.tags.push_back(tag.asString());
                    }
                }
                
                presets[preset.name] = preset;
            }
        } catch (const Json::Exception&) {
            // 파싱 오류 시 기본 프리셋 유지
        }
    }
    
    void SavePresets() {
        Json::Value root;
        
        for (const auto& pair : presets) {
            const auto& preset = pair.second;
            Json::Value presetJson;
            
            presetJson["name"] = preset.name;
            presetJson["description"] = preset.description;
            presetJson["targetFPS"] = preset.targetFPS;
            presetJson["category"] = preset.category;
            presetJson["requiresWarning"] = preset.requiresWarning;
            presetJson["warningMessage"] = preset.warningMessage;
            presetJson["createdTime"] = preset.createdTime;
            presetJson["lastUsed"] = preset.lastUsed;
            presetJson["useCount"] = preset.useCount;
            
            for (const std::string& tag : preset.tags) {
                presetJson["tags"].append(tag);
            }
            
            root[preset.name] = presetJson;
        }
        
        std::ofstream file(presetsFile);
        if (file.is_open()) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &file);
            file.close();
        }
    }
    
    void LoadCollections() {
        std::ifstream file(collectionsFile);
        if (!file.is_open()) return;
        
        Json::Value root;
        try {
            file >> root;
            file.close();
            
            for (const auto& collectionName : root.getMemberNames()) {
                const Json::Value& collectionJson = root[collectionName];
                
                PresetCollection collection;
                collection.name = collectionJson["name"].asString();
                collection.description = collectionJson["description"].asString();
                collection.author = collectionJson["author"].asString();
                collection.version = collectionJson["version"].asString();
                collection.createdTime = collectionJson["createdTime"].asInt64();
                
                if (collectionJson.isMember("presetNames")) {
                    for (const auto& presetName : collectionJson["presetNames"]) {
                        collection.presetNames.push_back(presetName.asString());
                    }
                }
                
                collections[collection.name] = collection;
            }
        } catch (const Json::Exception&) {
            // 파싱 오류 무시
        }
    }
    
    void SaveCollections() {
        Json::Value root;
        
        for (const auto& pair : collections) {
            const auto& collection = pair.second;
            Json::Value collectionJson;
            
            collectionJson["name"] = collection.name;
            collectionJson["description"] = collection.description;
            collectionJson["author"] = collection.author;
            collectionJson["version"] = collection.version;
            collectionJson["createdTime"] = collection.createdTime;
            
            for (const std::string& presetName : collection.presetNames) {
                collectionJson["presetNames"].append(presetName);
            }
            
            root[collection.name] = collectionJson;
        }
        
        std::ofstream file(collectionsFile);
        if (file.is_open()) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &file);
            file.close();
        }
    }
    
    void LoadHistory() {
        std::ifstream file(historyFile);
        if (!file.is_open()) return;
        
        Json::Value root;
        try {
            file >> root;
            file.close();
            
            for (const auto& recordJson : root) {
                ApplicationResult result;
                result.success = recordJson["success"].asBool();
                result.message = recordJson["message"].asString();
                result.previousFPS = recordJson["previousFPS"].asFloat();
                result.newFPS = recordJson["newFPS"].asFloat();
                
                int64_t timestamp = recordJson["timestamp"].asInt64();
                result.timestamp = std::chrono::system_clock::from_time_t(timestamp);
                
                applicationHistory.push_back(result);
            }
        } catch (const Json::Exception&) {
            // 파싱 오류 무시
        }
    }
    
    void SaveHistory() {
        Json::Value root;
        
        // 최근 100개 기록만 저장
        size_t start = applicationHistory.size() > 100 ? applicationHistory.size() - 100 : 0;
        
        for (size_t i = start; i < applicationHistory.size(); ++i) {
            const auto& result = applicationHistory[i];
            Json::Value recordJson;
            
            recordJson["success"] = result.success;
            recordJson["message"] = result.message;
            recordJson["previousFPS"] = result.previousFPS;
            recordJson["newFPS"] = result.newFPS;
            recordJson["timestamp"] = std::chrono::system_clock::to_time_t(result.timestamp);
            
            root.append(recordJson);
        }
        
        std::ofstream file(historyFile);
        if (file.is_open()) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &file);
            file.close();
        }
    }
};

int main() {
    std::wcout << L"=== FPS 프리셋 시스템 ===" << std::endl;
    std::wcout << L"다양한 FPS 설정을 저장하고 불러올 수 있습니다." << std::endl;
    
    FPSPresetSystem presetSystem;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 시스템 초기화
    if (!presetSystem.Initialize(processName)) {
        std::wcout << L"시스템 초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 모든 프리셋 보기" << std::endl;
        std::wcout << L"2. 프리셋 적용" << std::endl;
        std::wcout << L"3. 커스텀 프리셋 생성" << std::endl;
        std::wcout << L"4. 프리셋 검색" << std::endl;
        std::wcout << L"5. 컬렉션 관리" << std::endl;
        std::wcout << L"6. 적용 기록 보기" << std::endl;
        std::wcout << L"7. 프리셋 내보내기/가져오기" << std::endl;
        std::wcout << L"8. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1:
                presetSystem.ShowAllPresets();
                break;
                
            case 2: {
                std::wcout << L"\n프리셋 이름을 입력하세요: ";
                std::wstring presetNameW;
                std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
                std::getline(std::wcin, presetNameW);
                std::string presetName(presetNameW.begin(), presetNameW.end());
                
                std::wcout << L"FPS 주소를 입력하세요 (16진수): 0x";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                presetSystem.ApplyPreset(presetName, address);
                break;
            }
            
            case 3:
                presetSystem.CreateCustomPreset();
                break;
                
            case 4:
                presetSystem.SearchPresets();
                break;
                
            case 5: {
                std::wcout << L"\n1. 컬렉션 보기  2. 컬렉션 생성" << std::endl;
                std::wcout << L"선택: ";
                int subChoice;
                std::wcin >> subChoice;
                
                if (subChoice == 1) {
                    presetSystem.ShowCollections();
                } else if (subChoice == 2) {
                    presetSystem.CreatePresetCollection();
                }
                break;
            }
            
            case 6:
                presetSystem.ShowApplicationHistory();
                break;
                
            case 7: {
                std::wcout << L"\n1. 내보내기  2. 가져오기" << std::endl;
                std::wcout << L"선택: ";
                int subChoice;
                std::wcin >> subChoice;
                
                if (subChoice == 1) {
                    presetSystem.ExportPresets();
                } else if (subChoice == 2) {
                    presetSystem.ImportPresets();
                }
                break;
            }
            
            case 8:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}