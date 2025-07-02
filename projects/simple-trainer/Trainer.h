#pragma once

class Trainer {
public:
    static void Initialize();
    static void Shutdown();

private:
    static void GameLoop();
};
