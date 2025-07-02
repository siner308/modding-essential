#include "GameOverlay.h"
#include "D3D11Hook.h"
#include <iostream>

void GameOverlay::Initialize() {
    std::cout << "Game Overlay Initializing..." << std::endl;
    D3D11Hook::Initialize();
}

void GameOverlay::Shutdown() {
    std::cout << "Game Overlay Shutting down..." << std::endl;
    D3D11Hook::Shutdown();
}
