/*
 * PAK 로더 예제
 *
 * 이 예제는 언리얼 엔진 게임을 위한 단순화된 PAK 로딩 메커니즘을 시뮬레이션합니다.
 * 파일 명명 규칙(예: 높은 우선순위를 위한 Z_ 접두사)의 중요성과
 * 이러한 가상 패키지에서 에셋을 로드하는 과정을 강조합니다.
 *
 * 주요 개념:
 * - PAK 파일 우선순위: 특정 접두사(예: Z_)가 있는 파일이 마지막에 로드되어,
 *   이전 에셋을 재정의하는 데 더 높은 우선순위를 갖는 방법.
 * - 가상 파일 시스템: 다양한 PAK 파일에서 에셋을 통합된
 *   가상 파일 시스템으로 로드하는 것을 시뮬레이션합니다.
 * - 에셋 재정의: 우선순위가 높은 PAK 파일이
 *   우선순위가 낮은 PAK 파일의 에셋을 재정의하는 방법을 보여줍니다.
 */

#include <iostream>
#include <string>
#include <vector>
#map>
#include <algorithm>
#include <sstream>

// --- 시뮬레이션된 PAK 파일 구조 ---
struct PakFile {
    std::string filename;
    int priority; // 낮은 값 = 낮은 우선순위, 높은 값 = 높은 우선순위
    std::map<std::string, std::string> assets; // 에셋 경로 -> 에셋 내용

    // 생성자를 단순화
    PakFile(const std::string& name, int p, const std::map<std::string, std::string>& a)
        : filename(name), priority(p), assets(a) {}
};

// --- 단순화된 PAK 로더 클래스 ---
class SimplePakLoader {
private:
    std::vector<PakFile> loadedPaks;

public:
    // 시뮬레이션된 PAK 파일을 시스템에 로드
    void LoadPak(const std::string& filename, int priority, const std::map<std::string, std::string>& assets) {
        loadedPaks.emplace_back(filename, priority, assets);
        // 우선순위에 따라 PAK 정렬. 높은 우선순위(큰 숫자)는 나중에 처리되어
        // 이전 에셋을 재정의할 수 있도록 합니다.
        std::sort(loadedPaks.begin(), loadedPaks.end(), [](const PakFile& a, const PakFile& b) {
            return a.priority < b.priority; // 우선순위 오름차순
        });
        std::cout << "[로더] PAK 로드됨: " << filename << " (우선순위: " << priority << ")" << std::endl;
    }

    // 가상 파일 시스템에서 에셋 검색
    std::string GetAsset(const std::string& assetPath) {
        // 역순(가장 높은 우선순위부터)으로 PAK을 반복하여 가장 높은 우선순위 에셋 찾기
        for (auto it = loadedPaks.rbegin(); it != loadedPaks.rend(); ++it) {
            if (it->assets.count(assetPath)) {
                std::cout << "[로더] 에셋 '" << assetPath << "'이(가) " << it->filename << "에서 발견됨" << std::endl;
                return it->assets[assetPath];
            }
        }
        std::cout << "[로더] 에셋 '" << assetPath << "'이(가) 로드된 PAK에서 발견되지 않음." << std::endl;
        return "에셋을 찾을 수 없음";
    }

    // 현재 로드된 PAK 및 우선순위 표시 유틸리티
    void ShowLoadedPaks() const {
        std::cout << "\n--- 현재 로드된 PAK (우선순위별) ---" << std::endl;
        for (const auto& pak : loadedPaks) {
            std::cout << "- " << pak.filename << " (우선순위: " << pak.priority << ")" << std::endl;
        }
        std::cout << "------------------------------------------" << std::endl;
    }
};

// --- 메인 함수 (데모/테스트용) ---
int main() {
    std::cout << "=== PAK 로더 시뮬레이션 ===" << std::endl;
    
    SimplePakLoader loader;

    // 게임의 원본 PAK 로드 시뮬레이션 (낮은 우선순위)
    loader.LoadPak("Game_Core.pak", 10, {
        {"Textures/Player.png", "원본 플레이어 텍스처"},
        {"Sounds/Music.ogg", "원본 게임 음악"},
        {"Models/Tree.fbx", "원본 나무 모델"}
    });

    loader.LoadPak("Game_DLC1.pak", 20, {
        {"Textures/Player.png", "DLC 플레이어 텍스처"}, // 원본 재정의
        {"Maps/NewMap.umap", "DLC 맵 데이터"}
    });

    // 모드 PAK 로드 시뮬레이션 (높은 우선순위, Z_ 접두사 규칙 사용)
    loader.LoadPak("Z_MyAwesomeMod.pak", 100, {
        {"Textures/Player.png", "멋진 모드 플레이어 텍스처"}, // DLC 및 원본 재정의
        {"Sounds/Music.ogg", "멋진 모드 음악 리믹스"}, // 원본 재정의
        {"UI/CustomHUD.png", "커스텀 HUD 요소"}
    });

    loader.ShowLoadedPaks();

    std::cout << "\n--- 에셋 검색 시뮬레이션 ---" << std::endl;
    std::cout << "플레이어 텍스처: " << loader.GetAsset("Textures/Player.png") << std::endl;
    std::cout << "게임 음악: " << loader.GetAsset("Sounds/Music.ogg") << std::endl;
    std::cout << "나무 모델: " << loader.GetAsset("Models/Tree.fbx") << std::endl;
    std::cout << "새 맵: " << loader.GetAsset("Maps/NewMap.umap") << std::endl;
    std::cout << "커스텀 HUD: " << loader.GetAsset("UI/CustomHUD.png") << std::endl;
    std::cout << "존재하지 않는 에셋: " << loader.GetAsset("Textures/NonExistent.png") << std::endl;

    std::cout << "\n시뮬레이션 완료. Enter를 눌러 종료하세요.";
    std::cin.ignore();
    std::cin.get();

    return 0;
}
