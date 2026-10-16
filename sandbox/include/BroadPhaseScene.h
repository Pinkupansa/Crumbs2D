#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <vector>

// Scène de validation de la broad phase :
//  - des cercles et des boîtes qui dérivent, un body déplaçable à la souris (clic gauche) ;
//  - les AABB en paire sont rouges, les autres grises, et chaque paire est reliée par un segment ;
//  - un recalcul par force brute, avec les mêmes filtres, doit donner le même nombre de paires.
// Cas pièges inclus : un body à deux colliders qui se chevauchent (aucune paire attendue entre eux)
// et deux bodies statiques qui se chevauchent (aucune paire attendue non plus).
class BroadPhaseScene : public SandboxScene
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
    void UpdateDrag();
    void CountBruteForcePairs();
    void DrawDiagnostics();

    SceneCamera m_Camera;
    Crumbs2D::World m_World;
    std::vector<Crumbs2D::Body> m_Bodies;
    Crumbs2D::Body m_DraggedBody;

    std::vector<std::pair<Crumbs2D::Collider, Crumbs2D::Collider>> m_Pairs;

    // Recomptage par force brute, détaillé pour diagnostiquer un filtre manquant.
    struct BruteForceCounts
    {
        size_t Overlapping = 0; // toutes les paires d'AABB qui se chevauchent
        size_t SameBody = 0;    // dont : deux colliders du même body
        size_t BothStatic = 0;  // dont : deux bodies statiques
        size_t Expected = 0;    // ce que la broad phase doit renvoyer
    };
    BruteForceCounts m_BruteForce;

    int m_BodyCount = 40;
    bool m_Paused = false;
    bool m_DrawPairLinks = true;
    float m_Accumulator = 0.0f;

    static constexpr float kHalfArea = 12.0f;
    static constexpr int kMaxStepsPerFrame = 4;
};