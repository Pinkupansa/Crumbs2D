#include "RLRenderer.h"
#include "SceneCamera.h"

#include "raylib.h"
#include "rlImGui.h"

#include <cmath>

namespace
{
    Camera2D s_Camera = {{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, SceneCamera::kDefaultZoom};

    // Seul endroit où l'axe Y est inversé pour raylib.
    Vector2 ToRaylib(glm::vec2 p) { return {p.x, -p.y}; }
    Color ToRaylib(DrawColor c) { return {c.r, c.g, c.b, c.a}; }
}

void RLRenderer::BeginFrame(DrawColor clearColor)
{
    BeginDrawing();
    ClearBackground(ToRaylib(clearColor));
}

void RLRenderer::EndFrame()
{
    EndDrawing();
}

glm::vec2 RLRenderer::GetViewportSize()
{
    return {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
}

void RLRenderer::SetCamera(const SceneCamera &camera)
{
    const glm::vec2 viewport = camera.GetViewport();
    s_Camera.offset = {viewport.x * 0.5f, viewport.y * 0.5f};
    s_Camera.target = ToRaylib(camera.GetPosition());
    s_Camera.zoom = camera.GetZoom();
}

void RLRenderer::BeginWorld()
{
    BeginMode2D(s_Camera);
}

void RLRenderer::EndWorld()
{
    EndMode2D();
}

void RLRenderer::DrawGrid()
{
    // Uniquement la partie visible, calculée dans le repère raylib
    // (la grille est symétrique, l'inversion de Y n'a pas d'importance).
    const Vector2 topLeft = GetScreenToWorld2D({0.0f, 0.0f}, s_Camera);
    const Vector2 bottomRight = GetScreenToWorld2D(
        {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
        s_Camera);

    const Color gridColor = {60, 60, 66, 255};

    for (float x = std::floor(topLeft.x); x <= bottomRight.x; x += 1.0f)
        DrawLineV({x, topLeft.y}, {x, bottomRight.y}, gridColor);

    for (float y = std::floor(topLeft.y); y <= bottomRight.y; y += 1.0f)
        DrawLineV({topLeft.x, y}, {bottomRight.x, y}, gridColor);

    DrawLineV({topLeft.x, 0.0f}, {bottomRight.x, 0.0f}, RED);
    DrawLineV({0.0f, topLeft.y}, {0.0f, bottomRight.y}, GREEN);
}

void RLRenderer::DrawCircle(glm::vec2 center, float radius, DrawColor color)
{
    DrawCircleLinesV(ToRaylib(center), radius, ToRaylib(color));
}

void RLRenderer::DrawSegment(glm::vec2 a, glm::vec2 b, DrawColor color)
{
    DrawLineV(ToRaylib(a), ToRaylib(b), ToRaylib(color));
}

void RLRenderer::DrawBox(glm::vec2 center, glm::vec2 halfExtents, float angle, DrawColor color)
{
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    const glm::vec2 local[4] = {
        {-halfExtents.x, -halfExtents.y},
        {halfExtents.x, -halfExtents.y},
        {halfExtents.x, halfExtents.y},
        {-halfExtents.x, halfExtents.y}};

    glm::vec2 world[4];
    for (int i = 0; i < 4; ++i)
    {
        world[i] = {
            center.x + c * local[i].x - s * local[i].y,
            center.y + s * local[i].x + c * local[i].y};
    }

    for (int i = 0; i < 4; ++i)
        DrawSegment(world[i], world[(i + 1) % 4], color);
}

void RLRenderer::BeginImGui()
{
    rlImGuiBegin();
}

void RLRenderer::EndImGui()
{
    rlImGuiEnd();
}