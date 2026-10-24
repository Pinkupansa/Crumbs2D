#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <vector>

// Scène de référence pour les performances.
// Affiche le détail du temps passé dans chaque étape du Step (via World::GetProfile),
// ainsi que le coût de la scène elle-même (wrap, dessin).
class StressScene : public SandboxScene
{
public:
    void OnInit() override;
    void OnUpdate(float dt) override;
    void OnDraw() override;
    void OnImGuiRender() override;

private:
    void CreateBodies();
    void WrapBodies();
    void UpdateCameraNavigation();

    SceneCamera m_Camera;
    Crumbs2D::World m_World;
    std::vector<Crumbs2D::Body> m_Bodies;

    // Réglages
    int m_BodyCount = 10000;
    float m_HalfArea = 50.0f;
    bool m_Paused = false;
    bool m_DrawColliders = true;
    bool m_DrawAABBs = false;
    bool m_UseGravity = false;

    // Pas fixe : sa durée est celle des SolverSettings du world.
    float m_Accumulator = 0.0f;
    static constexpr int kMaxStepsPerFrame = 4;
    static constexpr float kHalfBox = 0.3f;

    // Mesures lissées, en millisecondes (temps moyen d'UN pas pour le profil du world)
    float m_FrameMs = 0.0f;
    float m_WrapMs = 0.0f;
    float m_DrawMs = 0.0f;
    uint32_t m_PairCount = 0;
    int m_StepsLastFrame = 0;
};