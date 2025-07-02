/*
 * PAK 모딩 예제
 *
 * 이 예제는 간단한 PAK 모드가 언리얼 엔진 게임과 어떻게 상호작용하는지 보여줍니다.
 * 실제 시나리오에서는 이 코드가 DLL로 컴파일된 후 .pak 파일로 패키징됩니다.
 * 그런 다음 .pak 파일은 게임 엔진에 의해 로드됩니다.
 *
 * 주요 개념:
 * - 게임 내 콘솔 명령어 등록 및 실행.
 * - 게임 상태와의 기본적인 상호작용 (개념적).
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>

// --- 개념적 게임 엔진 API (시뮬레이션) ---

// 간단한 게임 상태 변수 표현
float GGameSpeed = 1.0f;
bool GGodModeEnabled = false;

// 게임 메모리에 쓰기를 시뮬레이션하는 함수
void WriteGameMemory(const std::string& addressName, float value) {
    std::cout << "[게임 메모리] " << value << "를 " << addressName << "에 쓰는 중" << std::endl;
    if (addressName == "GameSpeed") {
        GGameSpeed = value;
    } else if (addressName == "GodMode") {
        GGodModeEnabled = (value != 0.0f);
    }
}

// 콘솔 명령어 콜백 타입
typedef std::function<void(const std::vector<std::string>& args)> ConsoleCommandCallback;

// 시뮬레이션된 콘솔 명령어 레지스트리
std::map<std::string, ConsoleCommandCallback> ConsoleCommandRegistry;

// 콘솔 명령어를 등록하는 시뮬레이션된 엔진 함수
void RegisterEngineConsoleCommand(const std::string& commandName, ConsoleCommandCallback callback) {
    ConsoleCommandRegistry[commandName] = callback;
    std::cout << "[엔진] 콘솔 명령어 등록됨: " << commandName << std::endl;
}

// 콘솔 명령어를 실행하는 시뮬레이션된 엔진 함수
void ExecuteEngineConsoleCommand(const std::string& commandLine) {
    std::cout << "[엔진] 명령어 실행 중: \"" << commandLine << "\"" << std::endl;
    
    std::stringstream ss(commandLine);
    std::string commandName;
    ss >> commandName;
    
    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    
    auto it = ConsoleCommandRegistry.find(commandName);
    if (it != ConsoleCommandRegistry.end()) {
        it->second(args);
    } else {
        std::cout << "[엔진] 알 수 없는 명령어: " << commandName << std::endl;
    }
}

// --- 모드 구현 ---

// 모드별 콘솔 명령어
void Mod_ToggleGodMode(const std::vector<std::string>& args) {
    GGodModeEnabled = !GGodModeEnabled;
    std::cout << "[모드] 무적 모드 토글: " << (GGodModeEnabled ? "활성화됨" : "비활성화됨") << std::endl;
    WriteGameMemory("GodMode", GGodModeEnabled ? 1.0f : 0.0f);
}

void Mod_SetGameSpeed(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "[모드] 사용법: set_game_speed <값>" << std::endl;
        return;
    }
    try {
        float speed = std::stof(args[0]);
        GGameSpeed = speed;
        std::cout << "[모드] 게임 속도 설정: " << GGameSpeed << std::endl;
        WriteGameMemory("GameSpeed", GGameSpeed);
    } catch (const std::exception& e) {
        std::cerr << "[모드] 유효하지 않은 속도 값: " << args[0] << std::endl;
    }
}

// 모드 진입점 (PAK 로딩 시뮬레이션용)
void OnModLoaded() {
    std::cout << "[모드] PakModExample 로드됨! 명령어 등록 중..." << std::endl;
    RegisterEngineConsoleCommand("toggle_god", Mod_ToggleGodMode);
    RegisterEngineConsoleCommand("set_game_speed", Mod_SetGameSpeed);
    std::cout << "[모드] 명령어 등록됨. 'toggle_god' 또는 'set_game_speed 0.5'를 시도해보세요." << std::endl;
}

void OnModUnloaded() {
    std::cout << "[모드] PakModExample 언로드됨. 게임 상태 복원 중..." << std::endl;
    // 실제 모드에서는 명령어를 등록 해제하고 원래 게임 상태를 복원합니다.
    WriteGameMemory("GodMode", 0.0f); // 언로드 시 무적 모드 비활성화
    WriteGameMemory("GameSpeed", 1.0f); // 언로드 시 게임 속도 재설정
}

// --- 메인 함수 (데모/테스트용) ---
int main() {
    std::cout << "=== PAK 모딩 예제 시뮬레이션 ===" << std::endl;
    
    // 모드 로딩 시뮬레이션
    OnModLoaded();
    
    // 게임 내 콘솔 명령어 시뮬레이션
    ExecuteEngineConsoleCommand("toggle_god");
    ExecuteEngineConsoleCommand("set_game_speed 0.5");
    ExecuteEngineConsoleCommand("toggle_god");
    ExecuteEngineConsoleCommand("set_game_speed 1.0");
    ExecuteEngineConsoleCommand("unknown_command");
    
    // 모드 언로드 시뮬레이션
    OnModUnloaded();
    
    std::cout << "\n시뮬레이션 완료. Enter를 눌러 종료하세요.";
    std::cin.ignore();
    std::cin.get();
    
    return 0;
}
