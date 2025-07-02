#include <Windows.h>
#include "ModLoader.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            ModLoader::GetInstance().LoadMods();
            break;
        case DLL_PROCESS_DETACH:
            ModLoader::GetInstance().UnloadMods();
            break;
    }
    return TRUE;
}
