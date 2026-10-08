#include "SceneCamera.h"
#include <algorithm>

void SceneCamera::SetViewport(glm::vec2 sizeInPixels)
{
    m_Viewport = sizeInPixels;
}

void SceneCamera::SetZoom(float pixelsPerMeter)
{
    m_Zoom = std::clamp(pixelsPerMeter, kMinZoom, kMaxZoom);
}

void SceneCamera::Pan(glm::vec2 screenDelta)
{
    // L'écran a Y vers le bas, le monde Y vers le haut : signe opposé sur Y.
    m_Position.x -= screenDelta.x / m_Zoom;
    m_Position.y += screenDelta.y / m_Zoom;
}

void SceneCamera::ZoomAt(glm::vec2 screenPoint, float factor)
{
    const glm::vec2 before = ScreenToWorld(screenPoint);
    SetZoom(m_Zoom * factor);
    const glm::vec2 after = ScreenToWorld(screenPoint);
    m_Position += before - after;
}

void SceneCamera::Reset()
{
    m_Position = {0.0f, 0.0f};
    m_Zoom = kDefaultZoom;
}

glm::vec2 SceneCamera::ScreenToWorld(glm::vec2 screen) const
{
    const glm::vec2 fromCenter = screen - m_Viewport * 0.5f;
    return {
        m_Position.x + fromCenter.x / m_Zoom,
        m_Position.y - fromCenter.y / m_Zoom};
}

glm::vec2 SceneCamera::WorldToScreen(glm::vec2 world) const
{
    const glm::vec2 fromCamera = world - m_Position;
    return {
        m_Viewport.x * 0.5f + fromCamera.x * m_Zoom,
        m_Viewport.y * 0.5f - fromCamera.y * m_Zoom};
}