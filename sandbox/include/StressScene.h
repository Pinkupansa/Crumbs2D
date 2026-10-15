#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <vector>

// Scène de référence pour les performances : N boîtes sans contacts.
// Les boîtes sortant de la zone réapparaissent de l'autre côté,
// pour garder une charge constante dans le temps.
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
    bool m_Paused = false;
    bool m_DrawBodies = true;
    bool m_UseGravity = false;

    // Pas fixe : sa durée est celle des SolverSettings du world.
    float m_Accumulator = 0.0f;
    static constexpr int kMaxStepsPerFrame = 4;
    static constexpr float kHalfArea = 50.0f;
    static constexpr float kHalfBox = 0.3f;

    // Mesures, lissées dans le temps (en millisecondes)
    float m_FrameMs = 0.0f;
    float m_StepMs = 0.0f;
    float m_WrapMs = 0.0f;
    float m_DrawMs = 0.0f;
    int m_StepsLastFrame = 0;
    // StressScene.h : deux options d'affichage de plus
    bool m_DrawColliders = true;
    bool m_DrawAABBs = false;
};