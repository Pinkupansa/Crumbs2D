#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <cstdint>
#include <vector>

// Scène de validation de la broad phase et de son arbre dynamique.
//  - Paires : comparées à un recomptage par force brute, avec diagnostic des filtres.
//  - Arbre : nœuds internes colorés par hauteur, AABB agrandies des feuilles, statistiques,
//    et appel à Validate() à chaque image.
// Clic gauche : déplacer la grande boîte. Clic droit : déplacer la caméra.
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
    void CollectTreeStats();
    void DrawPairDiagnostics();
    void DrawTreeDiagnostics();

    SceneCamera m_Camera;
    Crumbs2D::World m_World;
    std::vector<Crumbs2D::Body> m_Bodies;
    Crumbs2D::Body m_DraggedBody;

    std::vector<std::pair<Crumbs2D::Collider, Crumbs2D::Collider>> m_Pairs;

    // Recomptage par force brute, détaillé pour diagnostiquer un filtre manquant.
    struct BruteForceCounts
    {
        size_t Overlapping = 0;
        size_t SameBody = 0;
        size_t BothStatic = 0;
        size_t Expected = 0;
    };
    BruteForceCounts m_BruteForce;

    // Statistiques de l'arbre, calculées via ForEachBroadPhaseNode.
    struct TreeStats
    {
        uint32_t Height = 0;
        uint32_t NodeCount = 0;
        uint32_t LeafCount = 0;
        float TotalCost = 0.0f;
        bool IsValid = true;
    };
    TreeStats m_Tree;

    // Réglages
    int m_BodyCount = 40;
    bool m_Paused = false;
    bool m_DrawColliders = true;
    bool m_DrawExactAABBs = false;
    bool m_DrawFatAABBs = true;
    bool m_DrawTreeNodes = true;
    bool m_DrawPairLinks = false;
    int m_MinDrawnHeight = 1;
    float m_Accumulator = 0.0f;

    static constexpr float kHalfArea = 12.0f;
    static constexpr int kMaxStepsPerFrame = 4;
};