# PAK 로더 예제 코드

이 디렉터리에는 게임 엔진이 `.pak` 파일을 로드하고 우선순위를 지정하는 방법에 초점을 맞춘 PAK 로더의 개념적인 구현을 보여주는 예제 코드가 포함되어 있습니다.

## PakLoaderExample.cpp

이 예제는 단순화된 PAK 로딩 메커니즘을 시뮬레이션하며, 파일 명명 규칙(예: 높은 우선순위를 위한 `Z_` 접두사)의 중요성과 이러한 가상 패키지에서 에셋을 로드하는 과정을 강조합니다.

### 주요 개념:

- **PAK 파일 우선순위**: 특정 접두사(예: `Z_`)가 있는 파일이 마지막에 로드되어 게임 에셋을 재정의하는 데 더 높은 우선순위를 갖는 방법.
- **가상 파일 시스템**: 다양한 PAK 파일에서 에셋을 통합된 가상 파일 시스템으로 로드하는 것을 시뮬레이션합니다.
- **에셋 재정의**: 우선순위가 높은 PAK 파일이 우선순위가 낮은 PAK 파일의 에셋을 재정의하는 방법을 보여줍니다.

### 사용법 (개념적):

1.  이 코드는 시뮬레이션이며 실제 `.pak` 파일과 직접 상호작용하지 않습니다.
2.  게임 엔진 내에서 PAK 로더의 내부 로직을 보여줍니다.
3.  `main` 함수는 로딩 순서와 에셋 재정의 동작을 보여줍니다.

```cpp
// 단순화된 PAK 로더 예제
struct PakFile {
    std::string filename;
    int priority;
    std::map<std::string, std::string> assets;
};

class SimplePakLoader {
public:
    void LoadPak(const std::string& filename, int priority, const std::map<std::string, std::string>& assets) {
        PakFile pak = {filename, priority, assets};
        loadedPaks.push_back(pak);
        // 우선순위에 따라 PAK 정렬 (높은 우선순위 = 나중에 로드)
        std::sort(loadedPaks.begin(), loadedPaks.end(), [](const PakFile& a, const PakFile& b) {
            return a.priority < b.priority;
        });
    }

    std::string GetAsset(const std::string& assetPath) {
        // 우선순위 역순으로 PAK을 반복하여 가장 높은 우선순위 에셋 찾기
        for (auto it = loadedPaks.rbegin(); it != loadedPaks.rend(); ++it) {
            if (it->assets.count(assetPath)) {
                return it->assets[assetPath];
            }
        }
        return ""; // 에셋을 찾을 수 없음
    }

private:
    std::vector<PakFile> loadedPaks;
};

// 사용 예제
SimplePakLoader loader;
loader.LoadPak("Game_P.pak", 10, {{"Textures/Player.png", "원본 플레이어 텍스처"}});
loader.LoadPak("MyMod_P.pak", 20, {{"Textures/Player.png", "모드 플레이어 텍스처"}});
std::cout << loader.GetAsset("Textures/Player.png"); // 출력: 모드 플레이어 텍스처
```
