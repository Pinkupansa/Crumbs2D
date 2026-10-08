#include "RLWindow.h"

#include "raylib.h"
#include "rlImGui.h"

void RLWindow::Init(int width, int height, const char *title, int targetFps)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(width, height, title);
    SetTargetFPS(targetFps);
    rlImGuiSetup(true);
}

void RLWindow::Shutdown()
{
    rlImGuiShutdown();
    CloseWindow();
}

bool RLWindow::ShouldClose()
{
    return WindowShouldClose();
}

float RLWindow::GetDeltaTime()
{
    return GetFrameTime();
}