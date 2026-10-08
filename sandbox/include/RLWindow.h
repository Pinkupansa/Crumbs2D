#pragma once

class RLWindow
{
public:
    static void Init(int width, int height, const char *title, int targetFps);
    static void Shutdown();

    static bool ShouldClose();
    static float GetDeltaTime();
};