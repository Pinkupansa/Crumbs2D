#include "raylib.h"

#ifdef CRUMBS2D_SANDBOX_HAS_IMGUI
#include "imgui.h"
#include "rlImGui.h"
#endif

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Crumbs2D Sandbox");
    SetTargetFPS(144);

#ifdef CRUMBS2D_SANDBOX_HAS_IMGUI
    rlImGuiSetup(true); // true = thème sombre
#endif

    // Paramètres du cercle, modifiables depuis ImGui
    float circleColor[3] = {1.0f, 0.6f, 0.2f};
    float radius = 80.0f;
    bool filled = false;

    while (!WindowShouldClose())
    {
        const Vector2 center = {
            static_cast<float>(GetScreenWidth()) * 0.5f,
            static_cast<float>(GetScreenHeight()) * 0.5f};

        const Color color = {
            static_cast<unsigned char>(circleColor[0] * 255.0f),
            static_cast<unsigned char>(circleColor[1] * 255.0f),
            static_cast<unsigned char>(circleColor[2] * 255.0f),
            255};

        BeginDrawing();
        ClearBackground({30, 30, 34, 255});

        // Nos dessins d'abord...
        if (filled)
            DrawCircleV(center, radius, color);
        else
            DrawCircleLinesV(center, radius, color);

        // ...puis ImGui par-dessus
#ifdef CRUMBS2D_SANDBOX_HAS_IMGUI
        rlImGuiBegin();

        ImGui::Begin("Crumbs2D");
        ImGui::Text("FPS : %d", GetFPS());
        ImGui::Separator();
        ImGui::ColorEdit3("Couleur", circleColor);
        ImGui::SliderFloat("Rayon", &radius, 5.0f, 300.0f);
        ImGui::Checkbox("Rempli", &filled);
        ImGui::End();

        rlImGuiEnd();
#endif

        EndDrawing();
    }

#ifdef CRUMBS2D_SANDBOX_HAS_IMGUI
    rlImGuiShutdown();
#endif

    CloseWindow();
    return 0;
}