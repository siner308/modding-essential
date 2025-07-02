#include <Windows.h>
#include "GameOverlay.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            GameOverlay::Initialize();
            break;
        case DLL_PROCESS_DETACH:
            GameOverlay::Shutdown();
            break;
    }
    return TRUE;
}
