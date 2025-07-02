#include "Trainer.h"
#include <iostream>
#include <Windows.h>
#include <thread>

// Function to write to game memory
template<typename T>
void WriteMemory(uintptr_t address, T value) {
    DWORD oldProtect;
    VirtualProtect((LPVOID)address, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
    *(T*)address = value;
    VirtualProtect((LPVOID)address, sizeof(T), oldProtect, &oldProtect);
}

// Function to read from game memory
template<typename T>
T ReadMemory(uintptr_t address) {
    return *(T*)address;
}

// Example: Infinite Health
void InfiniteHealth() {
    // This address is just a placeholder. In a real scenario, you'd find this dynamically.
    uintptr_t healthAddress = 0xDEADBEEF; 
    int newHealth = 9999;
    WriteMemory<int>(healthAddress, newHealth);
    std::cout << "Health set to: " << newHealth << std::endl;
}

void Trainer::GameLoop() {
    while (true) {
        // Example: Toggle Infinite Health with F1 key
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            InfiniteHealth();
            // Debounce the key press
            Sleep(200);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Trainer::Initialize() {
    std::cout << "Trainer Initialized" << std::endl;
    // Start the game loop in a separate thread
    std::thread(GameLoop).detach();
}

void Trainer::Shutdown() {
    std::cout << "Trainer Shutdown" << std::endl;
}
