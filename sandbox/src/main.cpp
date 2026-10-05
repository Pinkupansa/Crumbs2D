#include <iostream>
#include "raylib.h"
int main()
{
    InitWindow(1200, 720, "Crumbs2D Sandbox");
    SetTargetFPS(144);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawCircleLines(600, 360, 50, GREEN);
        EndDrawing();
    }
    CloseWindow();

    return 0;
}