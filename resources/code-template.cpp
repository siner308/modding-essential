/*
 * 범용 모드/DLL 템플릿
 * 
 * 이 템플릿은 게임에 주입될 수 있는 C++ DLL의 기본 구조를 제공합니다.
 * 프로세스 연결/분리 이벤트를 위한 DllMain과 간단한 초기화/종료 흐름을 포함합니다.
 *
 * 게임 모딩을 시작하는 데 이 템플릿을 활용하세요.
 */

#include <Windows.h>
#include <iostream>
#include <codecvt>
#include <locale>

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// DLL이 프로세스에 연결될 때 호출되는 함수
void InitializeMod() {
    // 디버깅 출력을 위한 콘솔 생성
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::wcout.clear();
    std::wcerr.clear();
    std::wcin.clear();

    std::wcout << StringToWString("[Mod Template] 모드 초기화됨!") << std::endl;
    // 여기에 모드 로직을 작성하세요
}

// DLL이 프로세스에서 분리될 때 호출되는 함수
void ShutdownMod() {
    std::wcout << StringToWString("[Mod Template] 모드 종료 중!") << std::endl;
    // 여기에 모드 리소스를 정리하세요

    // 콘솔 해제
    FreeConsole();
}

// DllMain은 DLL의 진입점입니다
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // DllMain과의 문제 방지를 위해 스레드 라이브러리 호출 비활성화
            DisableThreadLibraryCalls(hModule);
            // 데드락 방지 및 더 복잡한 작업을 허용하기 위해 새 스레드에서 초기화 함수 호출
            CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)InitializeMod, hModule, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            ShutdownMod();
            break;
    }
    return TRUE;
}
