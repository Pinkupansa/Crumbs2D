#pragma once

#include <cstdint>
#include <glm/vec2.hpp>

class SceneCamera;

struct DrawColor
{
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

class RLRenderer
{
public:
    static void BeginFrame(DrawColor clearColor);
    static void EndFrame();

    static glm::vec2 GetViewportSize();

    // Recopie l'état d'une caméra dans la caméra interne du renderer.
    static void SetCamera(const SceneCamera &camera);

    static void BeginWorld();
    static void EndWorld();

    static void DrawGrid();
    static void DrawCircle(glm::vec2 center, float radius, DrawColor color);
    static void DrawSegment(glm::vec2 a, glm::vec2 b, DrawColor color);
    static void DrawBox(glm::vec2 center, glm::vec2 halfExtents, float angle, DrawColor color);

    static void BeginImGui();
    static void EndImGui();
};