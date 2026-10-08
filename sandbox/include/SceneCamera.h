#pragma once

#include <glm/vec2.hpp>

// Caméra possédée par une scène. Pure mathématique : aucune dépendance graphique.
// Coordonnées monde : mètres, Y vers le haut. Coordonnées écran : pixels, Y vers le bas.
class SceneCamera
{
public:
    void SetViewport(glm::vec2 sizeInPixels);
    glm::vec2 GetViewport() const { return m_Viewport; }

    void SetPosition(glm::vec2 position) { m_Position = position; }
    glm::vec2 GetPosition() const { return m_Position; }

    void SetZoom(float pixelsPerMeter);
    float GetZoom() const { return m_Zoom; }

    // Déplace la caméra d'un déplacement exprimé en pixels écran.
    void Pan(glm::vec2 screenDelta);

    // Multiplie le zoom en gardant fixe le point du monde sous screenPoint.
    void ZoomAt(glm::vec2 screenPoint, float factor);

    void Reset();

    glm::vec2 ScreenToWorld(glm::vec2 screen) const;
    glm::vec2 WorldToScreen(glm::vec2 world) const;

    static constexpr float kDefaultZoom = 50.0f;
    static constexpr float kMinZoom = 2.0f;
    static constexpr float kMaxZoom = 2000.0f;

private:
    glm::vec2 m_Viewport = {1280.0f, 720.0f};
    glm::vec2 m_Position = {0.0f, 0.0f};
    float m_Zoom = kDefaultZoom;
};