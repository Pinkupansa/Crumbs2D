#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <vector>

// Scène de validation du solveur de contacts, avec gravité.
// Préréglages : une boîte, une pile, une pyramide, un mélange de formes.
// Clic gauche : ajouter une forme. Clic droit : déplacer la caméra.
class SolverScene : public SandboxScene
{
public:
    void OnInit() override;
    void OnUpdate(float dt) override;
    void OnDraw() override;
    void OnImGuiRender() override;

private:
    enum class Preset { SingleBox, Stack, Pyramid, Mixed };

    void RebuildWorld();
    void CreateGround();
    Crumbs2D::Body AddBox(glm::vec2 position, glm::vec2 halfExtents, float rotation = 0.0f);
    Crumbs2D::Body AddCircle(glm::vec2 position, float radius);
    void SpawnAtMouse();
    void UpdateCameraNavigation();
    void CollectStats();

    SceneCamera m_Camera;
    Crumbs2D::World m_World;
    std::vector<Crumbs2D::Body> m_Bodies;
    std::vector<Crumbs2D::ContactInfo> m_Contacts;

    // Réglages du solveur, appliqués au prochain reset (fixés à la construction du world).
    int m_Iterations = 10;
    float m_Beta = 0.2f;
    float m_Slop = 0.005f;

    Preset m_Preset = Preset::Stack;
    int m_StackHeight = 10;
    bool m_Paused = false;
    bool m_DrawContacts = true;
    bool m_SpawnCircles = false;
    bool m_SpawnTrigger = false;
    bool m_SpawnStatic = false;
    float m_Accumulator = 0.0f;

    // Statistiques
    size_t m_PointCount = 0;
    float m_MaxPenetration = 0.0f;
    float m_MaxSpeed = 0.0f;

    static constexpr int kMaxStepsPerFrame = 4;
};