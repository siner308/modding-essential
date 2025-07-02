#include <Windows.h>
#include "Trainer.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            Trainer::Initialize();
            break;
        case DLL_PROCESS_DETACH:
            Trainer::Shutdown();
            break;
    }
    return TRUE;
}
