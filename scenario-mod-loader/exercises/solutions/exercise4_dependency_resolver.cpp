/*
 * Exercise 4: 의존성 해결
 * 
 * 문제: 모드 간 의존성을 분석하고 올바른 순서로 로드하는 시스템을 구현하세요.
 * 
 * 학습 목표:
 * - 그래프 이론 적용
 * - 위상 정렬 알고리즘
 * - 순환 참조 탐지
 */

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>
#include <stack>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <mutex>
#include <optional>
#include <regex>
#include <codecvt>
#include <locale>

namespace fs = std::filesystem;

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Helper function to convert std::wstring to std::string
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// 모드 정보 구조체
struct ModInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string filename;
    
    // 의존성 정보
    std::vector<std::string> requiredMods;        // 필수 의존성
    std::vector<std::string> optionalMods;        // 선택적 의존성
    std::vector<std::string> conflictMods;        // 충돌 모드
    std::vector<std::string> loadAfterMods;       // 이후 로드
    std::vector<std::string> loadBeforeMods;      // 이전 로드
    
    // 버전 제약
    std::map<std::string, std::string> versionConstraints;  // modName -> version constraint
    
    // 플랫폼 및 호환성
    std::vector<std::string> supportedPlatforms;
    std::string minimumGameVersion;
    std::string maximumGameVersion;
    
    // 우선순위 (낮을수록 먼저 로드)
    int loadPriority = 100;
    
    // 카테고리/태그
    std::vector<std::string> categories;
    std::vector<std::string> tags;
    
    ModInfo() = default;
    
    ModInfo(const std::string& modName, const std::string& modVersion = "1.0.0") 
        : name(modName), version(modVersion) {}
};

// 버전 비교 클래스
class VersionComparator {
public:
    struct Version {
        int major = 0;
        int minor = 0;
        int patch = 0;
        std::string suffix;  // alpha, beta, rc 등
        
        Version() = default;
        
        explicit Version(const std::string& versionStr) {
            ParseVersion(versionStr);
        }
        
        void ParseVersion(const std::string& versionStr) {
            std::regex versionRegex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?(?:-(.+))?)");
            std::smatch match;
            
            if (std::regex_match(versionStr, match, versionRegex)) {
                major = std::stoi(match[1].str());
                if (match[2].matched) minor = std::stoi(match[2].str());
                if (match[3].matched) patch = std::stoi(match[3].str());
                if (match[4].matched) suffix = match[4].str();
            }
        }
        
        bool operator<(const Version& other) const {
            if (major != other.major) return major < other.major;
            if (minor != other.minor) return minor < other.minor;
            if (patch != other.patch) return patch < other.patch;
            
            // suffix 처리 (alpha < beta < rc < stable)
            if (suffix.empty() && !other.suffix.empty()) return false;
            if (!suffix.empty() && other.suffix.empty()) return true;
            return suffix < other.suffix;
        }
        
        bool operator==(const Version& other) const {
            return major == other.major && minor == other.minor && 
                   patch == other.patch && suffix == other.suffix;
        }
        
        bool operator<=(const Version& other) const {
            return *this < other || *this == other;
        }
        
        bool operator>(const Version& other) const {
            return !(*this <= other);
        }
        
        bool operator>=(const Version& other) const {
            return !(*this < other);
        }
        
        std::string ToString() const {
            std::string result = std::to_string(major) + "." +
                               std::to_string(minor) + "." +
                               std::to_string(patch);
            if (!suffix.empty()) {
                result += "-" + suffix;
            }
            return result;
        }
    };
    
    static bool SatisfiesConstraint(const std::string& version, const std::string& constraint) {
        if (constraint.empty()) return true;
        
        Version ver(version);
        
        // 제약 조건 파싱 (>=1.0.0, >2.0.0, <=3.0.0, <4.0.0, ==1.2.3)
        std::regex constraintRegex(R"((>=|<=|>|<|==)\s*(.+))");
        std::smatch match;
        
        if (std::regex_match(constraint, match, constraintRegex)) {
            std::string op = match[1].str();
            Version constraintVer(match[2].str());
            
            if (op == ">=") return ver >= constraintVer;
            if (op == "<=") return ver <= constraintVer;
            if (op == ">") return ver > constraintVer;
            if (op == "<") return ver < constraintVer;
            if (op == "==") return ver == constraintVer;
        }
        
        // 단순 버전 매칭
        return Version(version) == Version(constraint);
    }
};

// 의존성 그래프 노드
struct DependencyNode {
    ModInfo modInfo;
    
    // 그래프 연결 정보
    std::vector<std::string> dependencies;    // 이 모드가 의존하는 모드들
    std::vector<std::string> dependents;      // 이 모드에 의존하는 모드들
    
    // 상태 정보
    enum class Status {
        NotVisited,
        Visiting,
        Visited,
        Resolved,
        Failed
    };
    
    Status status = Status::NotVisited;
    std::string errorMessage;
    
    // 로드 순서 (위상 정렬 결과)
    int loadOrder = -1;
    
    DependencyNode() = default;
    explicit DependencyNode(const ModInfo& info) : modInfo(info) {}
};

// 의존성 해결 결과
struct DependencyResolutionResult {
    bool success = false;
    std::vector<std::string> loadOrder;
    std::vector<std::string> missingDependencies;
    std::vector<std::string> circularDependencies;
    std::vector<std::string> conflictingMods;
    std::vector<std::string> versionMismatches;
    std::vector<std::string> warnings;
    std::vector<std::string> ignoredMods;
    
    // 성능 정보
    std::chrono::milliseconds resolutionTime{0};
    int totalMods = 0;
    int resolvedMods = 0;
    
    void PrintSummary() const {
        std::wcout << L"\n=== Dependency Resolution Summary ===" << std::endl;
        std::wcout << L"Success: " << (success ? L"Yes" : L"No") << std::endl;
        std::wcout << L"Total mods: " << totalMods << std::endl;
        std::wcout << L"Resolved mods: " << resolvedMods << std::endl;
        std::wcout << L"Resolution time: " << resolutionTime.count() << L"ms" << std::endl;
        
        if (!loadOrder.empty()) {
            std::wcout << L"\nLoad order (" << loadOrder.size() << L" mods):" << std::endl;
            for (size_t i = 0; i < loadOrder.size(); ++i) {
                std::wcout << L"  " << (i + 1) << L". " << StringToWString(loadOrder[i]) << std::endl;
            }
        }
        
        if (!missingDependencies.empty()) {
            std::wcout << L"\nMissing dependencies:" << std::endl;
            for (const auto& dep : missingDependencies) {
                std::wcout << L"  - " << StringToWString(dep) << std::endl;
            }
        }
        
        if (!circularDependencies.empty()) {
            std::wcout << L"\nCircular dependencies detected:" << std::endl;
            for (const auto& cycle : circularDependencies) {
                std::wcout << L"  - " << StringToWString(cycle) << std::endl;
            }
        }
        
        if (!conflictingMods.empty()) {
            std::wcout << L"\nConflicting mods:" << std::endl;
            for (const auto& conflict : conflictingMods) {
                std::wcout << L"  - " << StringToWString(conflict) << std::endl;
            }
        }
        
        if (!versionMismatches.empty()) {
            std::wcout << L"\nVersion mismatches:" << std::endl;
            for (const auto& mismatch : versionMismatches) {
                std::wcout << L"  - " << StringToWString(mismatch) << std::endl;
            }
        }
        
        if (!warnings.empty()) {
            std::wcout << L"\nWarnings:" << std::endl;
            for (const auto& warning : warnings) {
                std::wcout << L"  - " << StringToWString(warning) << std::endl;
            }
        }
        
        if (!ignoredMods.empty()) {
            std::wcout << L"\nIgnored mods:" << std::endl;
            for (const auto& ignored : ignoredMods) {
                std::wcout << L"  - " << StringToWString(ignored) << std::endl;
            }
        }
        
        std::wcout << L"=====================================" << std::endl;
    }
};

class DependencyResolver {
private:
    std::unordered_map<std::string, DependencyNode> nodes;
    std::vector<ModInfo> availableMods;
    
    // 설정
    bool allowOptionalDependencies = true;
    bool ignoreVersionConstraints = false;
    bool allowConflictingMods = false;
    bool strictDependencyCheck = true;
    
    // 통계
    mutable std::mutex statsMutex;
    int totalResolutions = 0;
    int successfulResolutions = 0;
    std::chrono::milliseconds totalResolutionTime{0};

public:
    DependencyResolver() = default;
    
    // 모드 정보 추가
    void AddMod(const ModInfo& modInfo) {
        nodes[modInfo.name] = DependencyNode(modInfo);
        availableMods.push_back(modInfo);
    }
    
    // 모드 정보 제거
    void RemoveMod(const std::string& modName) {
        nodes.erase(modName);
        availableMods.erase(
            std::remove_if(availableMods.begin(), availableMods.end(),
                [&modName](const ModInfo& info) { return info.name == modName; }),
            availableMods.end());
    }
    
    // 파일에서 모드 정보 로드
    bool LoadModsFromDirectory(const std::string& directory) {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            std::wcerr << L"Invalid directory: " << StringToWString(directory) << std::endl;
            return false;
        }
        
        int loadedCount = 0;
        
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                
                if (extension == ".dll") {
                    // DLL과 함께 있는 메타데이터 파일 찾기
                    std::string metaFile = entry.path().stem().string() + ".meta";
                    fs::path metaPath = entry.path().parent_path() / metaFile;
                    
                    if (fs::exists(metaPath)) {
                        if (LoadModMetadata(metaPath.string())) {
                            loadedCount++;
                        }
                    } else {
                        // 기본 정보로 모드 추가
                        ModInfo info;
                        info.name = entry.path().stem().string();
                        info.filename = entry.path().filename().string();
                        info.version = "1.0.0";
                        AddMod(info);
                        loadedCount++;
                    }
                } else if (extension == ".meta" || extension == ".ini") {
                    if (LoadModMetadata(entry.path().string())) {
                        loadedCount++;
                    }
                }
            }
        }
        
        std::wcout << L"Loaded " << loadedCount << L" mod(s) from " << StringToWString(directory) << std::endl;
        return loadedCount > 0;
    }
    
    // 메타데이터 파일 파싱
    bool LoadModMetadata(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::wcerr << L"Failed to open metadata file: " << StringToWString(filename) << std::endl;
            return false;
        }
        
        ModInfo modInfo;
        std::string line;
        std::string currentSection;
        
        while (std::getline(file, line)) {
            line = TrimString(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            
            // 섹션 처리
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }
            
            // 키=값 처리
            auto equalPos = line.find('=');
            if (equalPos == std::string::npos) continue;
            
            std::string key = TrimString(line.substr(0, equalPos));
            std::string value = TrimString(line.substr(equalPos + 1));
            
            // 기본 정보
            if (currentSection.empty() || currentSection == "General") {
                if (key == "name") modInfo.name = value;
                else if (key == "version") modInfo.version = value;
                else if (key == "author") modInfo.author = value;
                else if (key == "description") modInfo.description = value;
                else if (key == "filename") modInfo.filename = value;
                else if (key == "load_priority") modInfo.loadPriority = std::stoi(value);
                else if (key == "minimum_game_version") modInfo.minimumGameVersion = value;
                else if (key == "maximum_game_version") modInfo.maximumGameVersion = value;
            }
            // 의존성 정보
            else if (currentSection == "Dependencies") {
                if (key == "required") {
                    modInfo.requiredMods = SplitString(value, ',');
                } else if (key == "optional") {
                    modInfo.optionalMods = SplitString(value, ',');
                } else if (key == "conflicts") {
                    modInfo.conflictMods = SplitString(value, ',');
                } else if (key == "load_after") {
                    modInfo.loadAfterMods = SplitString(value, ',');
                } else if (key == "load_before") {
                    modInfo.loadBeforeMods = SplitString(value, ',');
                } else {
                    // 버전 제약 (dependency_name = >=1.0.0)
                    modInfo.versionConstraints[key] = value;
                }
            }
            // 호환성 정보
            else if (currentSection == "Compatibility") {
                if (key == "platforms") {
                    modInfo.supportedPlatforms = SplitString(value, ',');
                }
            }
            // 분류 정보
            else if (currentSection == "Metadata") {
                if (key == "categories") {
                    modInfo.categories = SplitString(value, ',');
                } else if (key == "tags") {
                    modInfo.tags = SplitString(value, ',');
                }
            }
        }
        
        file.close();
        
        if (!modInfo.name.empty()) {
            AddMod(modInfo);
            return true;
        }
        
        return false;
    }
    
    // 의존성 해결 실행
    DependencyResolutionResult ResolveDependencies(const std::vector<std::string>& requestedMods = {}) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        DependencyResolutionResult result;
        result.totalMods = static_cast<int>(nodes.size());
        
        try {
            // 1. 의존성 그래프 구성
            if (!BuildDependencyGraph(result)) {
                result.success = false;
                return result;
            }
            
            // 2. 충돌 검사
            if (!CheckConflicts(result)) {
                if (!allowConflictingMods) {
                    result.success = false;
                    return result;
                }
            }
            
            // 3. 버전 호환성 검사
            if (!CheckVersionCompatibility(result)) {
                if (!ignoreVersionConstraints) {
                    result.success = false;
                    return result;
                }
            }
            
            // 4. 순환 의존성 검사
            if (!CheckCircularDependencies(result)) {
                result.success = false;
                return result;
            }
            
            // 5. 위상 정렬 수행
            if (!PerformTopologicalSort(result, requestedMods)) {
                result.success = false;
                return result;
            }
            
            result.success = true;
            result.resolvedMods = static_cast<int>(result.loadOrder.size());
            
        } catch (const std::exception& e) {
            result.warnings.push_back("Exception during resolution: " + std::string(e.what()));
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.resolutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 통계 업데이트
        UpdateStatistics(result);
        
        return result;
    }
    
    // 특정 모드의 의존성 트리 출력
    void PrintDependencyTree(const std::string& modName, int depth = 0) const {
        auto it = nodes.find(modName);
        if (it == nodes.end()) {
            std::wcout << StringToWString(std::string(depth * 2, ' ') + "❌ " + modName + " (not found)") << std::endl;
            return;
        }
        
        const auto& node = it->second;
        std::wcout << StringToWString(std::string(depth * 2, ' ') + "📦 " + modName + " v" + node.modInfo.version) << std::endl;
        
        // 필수 의존성
        for (const auto& dep : node.modInfo.requiredMods) {
            std::wcout << StringToWString(std::string((depth + 1) * 2, ' ') + "🔗 " + dep + " (required)") << std::endl;
            if (depth < 3) {  // 무한 재귀 방지
                PrintDependencyTree(dep, depth + 2);
            }
        }
        
        // 선택적 의존성
        if (allowOptionalDependencies) {
            for (const auto& dep : node.modInfo.optionalMods) {
                std::wcout << StringToWString(std::string((depth + 1) * 2, ' ') + "🔗 " + dep + " (optional)") << std::endl;
                if (depth < 3 && nodes.find(dep) != nodes.end()) {
                    PrintDependencyTree(dep, depth + 2);
                }
            }
        }
    }
    
    // 설정 변경
    void SetAllowOptionalDependencies(bool allow) { allowOptionalDependencies = allow; }
    void SetIgnoreVersionConstraints(bool ignore) { ignoreVersionConstraints = ignore; }
    void SetAllowConflictingMods(bool allow) { allowConflictingMods = allow; }
    void SetStrictDependencyCheck(bool strict) { strictDependencyCheck = strict; }
    
    // 통계 출력
    void PrintStatistics() const {
        std::lock_guard<std::mutex> lock(statsMutex);
        
        std::wcout << L"\n=== Dependency Resolver Statistics ===" << std::endl;
        std::wcout << L"Total resolutions: " << totalResolutions << std::endl;
        std::wcout << L"Successful resolutions: " << successfulResolutions << std::endl;
        std::wcout << L"Success rate: " << (totalResolutions > 0 ? 
            (successfulResolutions * 100.0 / totalResolutions) : 0.0) << L"%" << std::endl;
        std::wcout << L"Average resolution time: " << (totalResolutions > 0 ? 
            (totalResolutionTime.count() / totalResolutions) : 0) << L"ms" << std::endl;
        std::wcout << L"Available mods: " << availableMods.size() << std::endl;
        std::wcout << L"=====================================" << std::endl;
    }
    
    // 모드 목록 출력
    void PrintAvailableMods() const {
        std::wcout << L"\n=== Available Mods ===" << std::endl;
        
        if (availableMods.empty()) {
            std::wcout << L"No mods available." << std::endl;
            return;
        }
        
        // 카테고리별로 정렬
        std::map<std::string, std::vector<const ModInfo*>> categorizedMods;
        
        for (const auto& mod : availableMods) {
            std::string category = mod.categories.empty() ? "Uncategorized" : mod.categories[0];
            categorizedMods[category].push_back(&mod);
        }
        
        for (const auto& [category, mods] : categorizedMods) {
            std::wcout << L"\n[" << StringToWString(category) << L"]" << std::endl;
            
            for (const auto* mod : mods) {
                std::wcout << L"  📦 " << StringToWString(mod->name) << L" v" << StringToWString(mod->version);
                if (!mod->author.empty()) {
                    std::wcout << L" by " << StringToWString(mod->author);
                }
                std::wcout << std::endl;
                
                if (!mod->description.empty()) {
                    std::wcout << L"     " << StringToWString(mod->description) << std::endl;
                }
                
                if (!mod->requiredMods.empty()) {
                    std::wcout << L"     Requires: ";
                    for (size_t i = 0; i < mod->requiredMods.size(); ++i) {
                        if (i > 0) std::wcout << L", ";
                        std::wcout << StringToWString(mod->requiredMods[i]);
                    }
                    std::wcout << std::endl;
                }
                
                if (!mod->conflictMods.empty()) {
                    std::wcout << L"     Conflicts: ";
                    for (size_t i = 0; i < mod->conflictMods.size(); ++i) {
                        if (i > 0) std::wcout << L", ";
                        std::wcout << StringToWString(mod->conflictMods[i]);
                    }
                    std::wcout << std::endl;
                }
                
                std::wcout << std::endl;
            }
        }
        
        std::wcout << L"=====================" << std::endl;
    }

private:
    bool BuildDependencyGraph(DependencyResolutionResult& result) {
        // 노드 초기화
        for (auto& [name, node] : nodes) {
            node.status = DependencyNode::Status::NotVisited;
            node.dependencies.clear();
            node.dependents.clear();
            node.loadOrder = -1;
        }
        
        // 의존성 연결 구성
        for (auto& [name, node] : nodes) {
            const auto& modInfo = node.modInfo;
            
            // 필수 의존성 추가
            for (const auto& dep : modInfo.requiredMods) {
                if (nodes.find(dep) == nodes.end()) {
                    result.missingDependencies.push_back(name + " requires " + dep);
                    if (strictDependencyCheck) {
                        return false;
                    }
                    continue;
                }
                
                node.dependencies.push_back(dep);
                nodes[dep].dependents.push_back(name);
            }
            
            // 선택적 의존성 추가 (모드가 존재하는 경우만)
            if (allowOptionalDependencies) {
                for (const auto& dep : modInfo.optionalMods) {
                    if (nodes.find(dep) != nodes.end()) {
                        node.dependencies.push_back(dep);
                        nodes[dep].dependents.push_back(name);
                    }
                }
            }
            
            // load_after 의존성 추가
            for (const auto& dep : modInfo.loadAfterMods) {
                if (nodes.find(dep) != nodes.end()) {
                    node.dependencies.push_back(dep);
                    nodes[dep].dependents.push_back(name);
                }
            }
            
            // load_before 의존성 추가 (역방향)
            for (const auto& dep : modInfo.loadBeforeMods) {
                if (nodes.find(dep) != nodes.end()) {
                    nodes[dep].dependencies.push_back(name);
                    node.dependents.push_back(dep);
                }
            }
        }
        
        return true;
    }
    
    bool CheckConflicts(DependencyResolutionResult& result) {
        for (const auto& [name, node] : nodes) {
            for (const auto& conflict : node.modInfo.conflictMods) {
                if (nodes.find(conflict) != nodes.end()) {
                    result.conflictingMods.push_back(name + " conflicts with " + conflict);
                }
            }
        }
        
        return result.conflictingMods.empty() || allowConflictingMods;
    }
    
    bool CheckVersionCompatibility(DependencyResolutionResult& result) {
        for (const auto& [name, node] : nodes) {
            for (const auto& [depName, constraint] : node.modInfo.versionConstraints) {
                auto it = nodes.find(depName);
                if (it != nodes.end()) {
                    const std::string& depVersion = it->second.modInfo.version;
                    
                    if (!VersionComparator::SatisfiesConstraint(depVersion, constraint)) {
                        result.versionMismatches.push_back(
                            name + " requires " + depName + " " + constraint +
                            " but " + depVersion + " is available"
                        );
                    }
                }
            }
        }
        
        return result.versionMismatches.empty() || ignoreVersionConstraints;
    }
    
    bool CheckCircularDependencies(DependencyResolutionResult& result) {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursionStack;
        
        for (const auto& [name, node] : nodes) {
            if (visited.find(name) == visited.end()) {
                std::vector<std::string> cycle;
                if (HasCycleDFS(name, visited, recursionStack, cycle)) {
                    std::string cycleStr = "Circular dependency: ";
                    for (size_t i = 0; i < cycle.size(); ++i) {
                        if (i > 0) cycleStr += " -> ";
                        cycleStr += cycle[i];
                    }
                    result.circularDependencies.push_back(cycleStr);
                }
            }
        }
        
        return result.circularDependencies.empty();
    }
    
    bool HasCycleDFS(const std::string& nodeName, 
                    std::unordered_set<std::string>& visited,
                    std::unordered_set<std::string>& recursionStack,
                    std::vector<std::string>& cycle) {
        
        visited.insert(nodeName);
        recursionStack.insert(nodeName);
        cycle.push_back(nodeName);
        
        auto it = nodes.find(nodeName);
        if (it != nodes.end()) {
            for (const auto& dep : it->second.dependencies) {
                if (recursionStack.find(dep) != recursionStack.end()) {
                    // 순환 발견
                    cycle.push_back(dep);
                    return true;
                }
                
                if (visited.find(dep) == visited.end()) {
                    if (HasCycleDFS(dep, visited, recursionStack, cycle)) {
                        return true;
                    }
                }
            }
        }
        
        recursionStack.erase(nodeName);
        cycle.pop_back();
        return false;
    }
    
    bool PerformTopologicalSort(DependencyResolutionResult& result, const std::vector<std::string>& requestedMods) {
        // Kahn's 알고리즘 사용
        std::unordered_map<std::string, int> inDegree;
        std::priority_queue<std::pair<int, std::string>, 
                           std::vector<std::pair<int, std::string>>,
                           std::greater<std::pair<int, std::string>>> pq;  // 우선순위 큐 (낮은 우선순위 먼저)
        
        // 요청된 모드만 처리하거나 모든 모드 처리
        std::unordered_set<std::string> modsToProcess;
        if (requestedMods.empty()) {
            for (const auto& [name, node] : nodes) {
                modsToProcess.insert(name);
            }
        } else {
            // 요청된 모드와 그 의존성들 포함
            for (const auto& modName : requestedMods) {
                CollectDependencies(modName, modsToProcess);
            }
        }
        
        // 진입 차수 계산
        for (const auto& modName : modsToProcess) {
            inDegree[modName] = 0;
        }
        
        for (const auto& modName : modsToProcess) {
            auto it = nodes.find(modName);
            if (it != nodes.end()) {
                for (const auto& dep : it->second.dependencies) {
                    if (modsToProcess.find(dep) != modsToProcess.end()) {
                        inDegree[modName]++;
                    }
                }
            }
        }
        
        // 진입 차수가 0인 노드들을 우선순위 큐에 추가
        for (const auto& [modName, degree] : inDegree) {
            if (degree == 0) {
                auto it = nodes.find(modName);
                int priority = (it != nodes.end()) ? it->second.modInfo.loadPriority : 100;
                pq.push({priority, modName});
            }
        }
        
        // 위상 정렬 수행
        while (!pq.empty()) {
            auto [priority, modName] = pq.top();
            pq.pop();
            
            result.loadOrder.push_back(modName);
            
            auto it = nodes.find(modName);
            if (it != nodes.end()) {
                // 이 모드에 의존하는 모드들의 진입 차수 감소
                for (const auto& dependent : it->second.dependents) {
                    if (modsToProcess.find(dependent) != modsToProcess.end()) {
                        inDegree[dependent]--;
                        if (inDegree[dependent] == 0) {
                            auto depIt = nodes.find(dependent);
                            int depPriority = (depIt != nodes.end()) ? depIt->second.modInfo.loadPriority : 100;
                            pq.push({depPriority, dependent});
                        }
                    }
                }
            }
        }
        
        // 모든 모드가 처리되었는지 확인
        if (result.loadOrder.size() != modsToProcess.size()) {
            result.warnings.push_back("Not all mods could be ordered (possible circular dependencies)");
            
            // 처리되지 않은 모드들 찾기
            std::unordered_set<std::string> processedMods(result.loadOrder.begin(), result.loadOrder.end());
            for (const auto& modName : modsToProcess) {
                if (processedMods.find(modName) == processedMods.end()) {
                    result.ignoredMods.push_back(modName);
                }
            }
        }
        
        return true;
    }
    
    void CollectDependencies(const std::string& modName, std::unordered_set<std::string>& collected) {
        if (collected.find(modName) != collected.end()) {
            return;  // 이미 처리됨
        }
        
        auto it = nodes.find(modName);
        if (it == nodes.end()) {
            return;  // 모드가 존재하지 않음
        }
        
        collected.insert(modName);
        
        // 의존성들도 재귀적으로 수집
        for (const auto& dep : it->second.dependencies) {
            CollectDependencies(dep, collected);
        }
    }
    
    void UpdateStatistics(const DependencyResolutionResult& result) {
        std::lock_guard<std::mutex> lock(statsMutex);
        
        totalResolutions++;
        if (result.success) {
            successfulResolutions++;
        }
        totalResolutionTime += result.resolutionTime;
    }
    
    std::vector<std::string> SplitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delimiter)) {
            token = TrimString(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    std::string TrimString(const std::string& str) {
        const char* whitespace = " \t\r\n";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }
};

// 테스트 모드 생성기
class TestModGenerator {
public:
    static void GenerateTestMods(const std::string& outputDir) {
        fs::create_directories(outputDir);
        
        // 테스트 모드들 정의
        std::vector<ModInfo> testMods = {
            CreateMod("CoreMod", "1.0.0", "Core system modifications", {}, {}, {}, 10),
            CreateMod("UIFramework", "2.1.0", "UI enhancement framework", {"CoreMod"}, {}, {}, 20),
            CreateMod("GraphicsEnhancer", "1.5.0", "Graphics improvements", {"CoreMod"}, {"UIFramework"}, {}, 30),
            CreateMod("SoundMod", "3.0.0", "Audio enhancements", {"CoreMod"}, {}, {"OldSoundMod"}, 25),
            CreateMod("GameplayTweaks", "1.2.3", "Gameplay modifications", {"CoreMod", "UIFramework"}, {"GraphicsEnhancer"}, {}, 40),
            CreateMod("AdvancedFeatures", "0.9.0-beta", "Advanced game features", {"GameplayTweaks", "SoundMod"}, {}, {}, 50),
            CreateMod("OptionalAddon", "1.0.0", "Optional addon", {}, {"AdvancedFeatures"}, {}, 60),
            CreateMod("ConflictingMod", "2.0.0", "Mod that conflicts", {"CoreMod"}, {}, {"GameplayTweaks"}, 35),
            CreateMod("IndependentMod", "1.1.0", "Standalone modification", {}, {}, {}, 70),
            CreateMod("LegacyMod", "0.5.0", "Legacy modification", {}, {}, {"AdvancedFeatures"}, 80)
        };
        
        // 메타데이터 파일 생성
        for (const auto& mod : testMods) {
            SaveModMetadata(mod, outputDir);
        }
        
        std::wcout << L"Generated " << testMods.size() << L" test mods in " << StringToWString(outputDir) << std::endl;
    }
    
private:
    static ModInfo CreateMod(const std::string& name, const std::string& version, 
                           const std::string& description,
                           const std::vector<std::string>& required,
                           const std::vector<std::string>& optional,
                           const std::vector<std::string>& conflicts,
                           int priority) {
        ModInfo mod;
        mod.name = name;
        mod.version = version;
        mod.description = description;
        mod.filename = name + ".dll";
        mod.requiredMods = required;
        mod.optionalMods = optional;
        mod.conflictMods = conflicts;
        mod.loadPriority = priority;
        mod.author = "Test Generator";
        mod.categories = {"Test", "Generated"};
        return mod;
    }
    
    static void SaveModMetadata(const ModInfo& mod, const std::string& outputDir) {
        fs::path metaPath = fs::path(outputDir) / (mod.name + ".meta");
        
        std::ofstream file(metaPath);
        if (!file.is_open()) return;
        
        file << "# Metadata for " << mod.name << "\n\n";
        
        file << "[General]\n";
        file << "name=" << mod.name << "\n";
        file << "version=" << mod.version << "\n";
        file << "author=" << mod.author << "\n";
        file << "description=" << mod.description << "\n";
        file << "filename=" << mod.filename << "\n";
        file << "load_priority=" << mod.loadPriority << "\n\n";
        
        file << "[Dependencies]\n";
        if (!mod.requiredMods.empty()) {
            file << "required=";
            for (size_t i = 0; i < mod.requiredMods.size(); ++i) {
                if (i > 0) file << ",";
                file << mod.requiredMods[i];
            }
            file << "\n";
        }
        
        if (!mod.optionalMods.empty()) {
            file << "optional=";
            for (size_t i = 0; i < mod.optionalMods.size(); ++i) {
                if (i > 0) file << ",";
                file << mod.optionalMods[i];
            }
            file << "\n";
        }
        
        if (!mod.conflictMods.empty()) {
            file << "conflicts=";
            for (size_t i = 0; i < mod.conflictMods.size(); ++i) {
                if (i > 0) file << ",";
                file << mod.conflictMods[i];
            }
            file << "\n";
        }
        
        file << "\n[Metadata]\n";
        if (!mod.categories.empty()) {
            file << "categories=";
            for (size_t i = 0; i < mod.categories.size(); ++i) {
                if (i > 0) file << ",";
                file << mod.categories[i];
            }
            file << "\n";
        }
        
        file.close();
    }
};

// 메인 테스트 프로그램
class DependencyTestProgram {
private:
    std::unique_ptr<DependencyResolver> resolver;
    bool running;
    
public:
    DependencyTestProgram() : running(false) {
        resolver = std::make_unique<DependencyResolver>();
    }
    
    void Run() {
        std::wcout << L"=== Dependency Resolver Test Program ===" << std::endl;
        
        running = true;
        
        std::wcout << L"Type 'help' for commands, 'quit' to exit" << std::endl;
        
        std::wstring inputW;
        while (running) {
            std::wcout << L"\ndep_resolver> ";
            std::getline(std::wcin, inputW);
            std::string input = WStringToString(inputW);
            
            ProcessCommand(input);
        }
    }
    
private:
    void ProcessCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
        if (command == "help") {
            ShowHelp();
        } else if (command == "load") {
            std::string directory;
            iss >> directory;
            if (directory.empty()) directory = "./test_mods";
            resolver->LoadModsFromDirectory(directory);
        } else if (command == "generate") {
            std::string directory;
            iss >> directory;
            if (directory.empty()) directory = "./test_mods";
            TestModGenerator::GenerateTestMods(directory);
        } else if (command == "list") {
            resolver->PrintAvailableMods();
        } else if (command == "resolve") {
            std::vector<std::string> requestedMods;
            std::string mod;
            while (iss >> mod) {
                requestedMods.push_back(mod);
            }
            
            auto result = resolver->ResolveDependencies(requestedMods);
            result.PrintSummary();
        } else if (command == "tree") {
            std::string modName;
            iss >> modName;
            if (!modName.empty()) {
                resolver->PrintDependencyTree(modName);
            } else {
                std::wcout << L"Usage: tree <mod_name>" << std::endl;
            }
        } else if (command == "stats") {
            resolver->PrintStatistics();
        } else if (command == "config") {
            ConfigureResolver();
        } else if (command == "benchmark") {
            RunBenchmark();
        } else if (command == "quit" || command == "exit") {
            running = false;
        } else if (!command.empty()) {
            std::wcout << L"Unknown command: " << StringToWString(command) << std::endl;
        }
    }
    
    void ShowHelp() {
        std::wcout << L"\nAvailable commands:" << std::endl;
        std::wcout << L"  help                    - Show this help" << std::endl;
        std::wcout << L"  load [directory]        - Load mods from directory" << std::endl;
        std::wcout << L"  generate [directory]    - Generate test mods" << std::endl;
        std::wcout << L"  list                    - List available mods" << std::endl;
        std::wcout << L"  resolve [mod1 mod2...]  - Resolve dependencies" << std::endl;
        std::wcout << L"  tree <mod_name>         - Show dependency tree" << std::endl;
        std::wcout << L"  stats                   - Show statistics" << std::endl;
        std::wcout << L"  config                  - Configure resolver settings" << std::endl;
        std::wcout << L"  benchmark               - Run performance benchmark" << std::endl;
        std::wcout << L"  quit/exit               - Exit program" << std::endl;
    }
    
    void ConfigureResolver() {
        std::wcout << L"\n=== Resolver Configuration ===" << std::endl;
        std::wcout << L"1. Allow optional dependencies" << std::endl;
        std::wcout << L"2. Ignore version constraints" << std::endl;
        std::wcout << L"3. Allow conflicting mods" << std::endl;
        std::wcout << L"4. Strict dependency check" << std::endl;
        std::wcout << L"Select option (1-4): ";
        
        int option;
        std::wcin >> option;
        
        switch (option) {
            case 1: {
                std::wcout << L"Enable optional dependencies? (y/n): ";
                wchar_t choice;
                std::wcin >> choice;
                resolver->SetAllowOptionalDependencies(choice == L'y' || choice == L'Y');
                break;
            }
            case 2: {
                std::wcout << L"Ignore version constraints? (y/n): ";
                wchar_t choice;
                std::wcin >> choice;
                resolver->SetIgnoreVersionConstraints(choice == L'y' || choice == L'Y');
                break;
            }
            case 3: {
                std::wcout << L"Allow conflicting mods? (y/n): ";
                wchar_t choice;
                std::wcin >> choice;
                resolver->SetAllowConflictingMods(choice == L'y' || choice == L'Y');
                break;
            }
            case 4: {
                std::wcout << L"Enable strict dependency check? (y/n): ";
                wchar_t choice;
                std::wcin >> choice;
                resolver->SetStrictDependencyCheck(choice == L'y' || choice == L'Y');
                break;
            }
            default:
                std::wcout << L"Invalid option" << std::endl;
        }
        
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');  // 버퍼 클리어
    }
    
    void RunBenchmark() {
        std::wcout << L"\nRunning benchmark..." << std::endl;
        
        const int iterations = 100;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            resolver->ResolveDependencies();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::wcout << L"Benchmark completed:" << std::endl;
        std::wcout << L"  Iterations: " << iterations << std::endl;
        std::wcout << L"  Total time: " << totalTime.count() << L"ms" << std::endl;
        std::wcout << L"  Average time: " << (totalTime.count() / iterations) << L"ms per resolution" << std::endl;
    }
};

// 메인 함수
int main() {
    try {
        DependencyTestProgram program;
        program.Run();
    } catch (const std::exception& e) {
        std::wcerr << L"Fatal error: " << StringToWString(e.what()) << std::endl;
        return 1;
    }
    
    return 0;
}