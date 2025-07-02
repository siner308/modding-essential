#pragma once
#include <vector>
#include <string>
#include "IMod.h"

class ModLoader {
public:
    static ModLoader& GetInstance();
    void LoadMods();
    void UnloadMods();

private:
    ModLoader() = default;
    std::vector<IMod*> m_mods;
};
