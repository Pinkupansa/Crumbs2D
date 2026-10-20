#include "BroadPhaseScene.h"

#include "CrumbsMath.h"
#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
const DrawColor kColliderColor = {90, 170, 255, 255};
const DrawColor kStaticColor = {150, 150, 150, 255};
const DrawColor kExactAabbColor = {255, 200, 60, 255};
const DrawColor kFatAabbColor = {120, 220, 120, 255};
const DrawColor kLinkColor = {255, 70, 70, 255};

glm::vec2 AabbCenter(const Crumbs2D::AABB& aabb) { return (aabb.min + aabb.max) * 0.5f; }
glm::vec2 AabbHalfSize(const Crumbs2D::AABB& aabb) { return (aabb.max - aabb.min) * 0.5f; }

void DrawAabb(const Crumbs2D::AABB& aabb, DrawColor color)
{ RLRenderer::DrawBox(AabbCenter(aabb), AabbHalfSize(aabb), 0.0f, color); }

// Dégradé du bleu (bas de l'arbre) au rouge (racine).
DrawColor HeightColor(uint32_t height, uint32_t maxHeight)
{
    const float t = maxHeight > 1 ? static_cast<float>(height - 1) / static_cast<float>(maxHeight - 1) : 1.0f;
    return {static_cast<uint8_t>(60.0f + 195.0f * t), 80, static_cast<uint8_t>(255.0f - 195.0f * t), 255};
}
} // namespace

void BroadPhaseScene::OnInit()
{
    m_Camera.SetZoom(30.0f);
    CreateBodies();
}

void BroadPhaseScene::CreateBodies()
{
    m_World = Crumbs2D::World{Crumbs2D::SolverSettings{1.0f / 60.0f}};
    m_World.PhysicsSettings.Gravity = {0.0f, 0.0f};
    m_Bodies.clear();
    m_DraggedBody = Crumbs2D::Body{};

    // Graine fixe : la même scène à chaque reset.
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> position(-kHalfArea, kHalfArea);
    std::uniform_real_distribution<float> speed(-1.5f, 1.5f);
    std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> size(0.3f, 1.0f);
    std::bernoulli_distribution isCircle(0.5);

    for (int i = 0; i < m_BodyCount; ++i)
    {
        Crumbs2D::Body body = m_World.CreateBody();
        body.SetPosition({position(rng), position(rng)});
        body.SetRotation(angle(rng));
        body.SetVelocity({speed(rng), speed(rng)});
        body.SetAngularVelocity(speed(rng));

        if (isCircle(rng))
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({size(rng)}), {0.0f, 0.0f}, 0.0f);
        else
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{size(rng), size(rng) * 0.6f}}), {0.0f, 0.0f}, 0.0f);

        m_Bodies.push_back(body);
    }

    // Piège 1 : un body à deux colliders qui se chevauchent -> aucune paire entre eux.
    Crumbs2D::Body compound = m_World.CreateBody();
    compound.SetPosition({-4.0f, 0.0f});
    compound.SetAngularVelocity(0.5f);
    compound.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.2f, 0.4f}}), {0.0f, 0.0f}, 0.0f);
    compound.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({0.6f}), {1.0f, 0.0f}, 0.0f);
    m_Bodies.push_back(compound);

    // Piège 2 : deux bodies statiques qui se chevauchent -> aucune paire entre eux.
    for (float x : {3.0f, 4.0f})
    {
        Crumbs2D::Body ground = m_World.CreateBody();
        ground.SetIsStatic(true);
        ground.SetPosition({x, -kHalfArea + 1.0f});
        ground.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.5f, 0.5f}}), {0.0f, 0.0f}, 0.0f);
        m_Bodies.push_back(ground);
    }

    // Le body déplaçable à la souris.
    m_DraggedBody = m_World.CreateBody();
    m_DraggedBody.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.5f, 1.0f}}), {0.0f, 0.0f}, 0.0f);
    m_Bodies.push_back(m_DraggedBody);

    m_Accumulator = 0.0f;

    // Un premier pas pour avoir des AABB, des paires et un arbre à afficher, même en pause.
    m_World.Step();
    m_Pairs = m_World.GetCandidatePairs();
    CountBruteForcePairs();
    CollectTreeStats();
}

void BroadPhaseScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);
    UpdateDrag();

    if (!m_Paused)
    {
        const float stepDuration = m_World.GetSolverSettings().DeltaTime;
        m_Accumulator += dt;

        int steps = 0;
        while (m_Accumulator >= stepDuration && steps < kMaxStepsPerFrame)
        {
            m_World.Step();
            WrapBodies();
            m_Accumulator -= stepDuration;
            ++steps;
        }
        if (steps == kMaxStepsPerFrame)
            m_Accumulator = 0.0f;
    }

    m_Pairs = m_World.GetCandidatePairs();
    CountBruteForcePairs();
    CollectTreeStats();
}

void BroadPhaseScene::UpdateDrag()
{
    if (RLInput::IsMouseCapturedByUI() || !RLInput::IsMouseButtonDown(InputMouseButton::Left))
        return;

    m_DraggedBody.SetPosition(m_Camera.ScreenToWorld(RLInput::GetMousePosition()));
    m_DraggedBody.SetVelocity({0.0f, 0.0f});
    m_DraggedBody.SetAngularVelocity(0.0f);
}

void BroadPhaseScene::WrapBodies()
{
    constexpr float size = 2.0f * kHalfArea;
    for (const Crumbs2D::Body& body : m_Bodies)
    {
        glm::vec2 p = body.GetPosition();
        const glm::vec2 before = p;
        if (p.x < -kHalfArea)
            p.x += size;
        else if (p.x > kHalfArea)
            p.x -= size;
        if (p.y < -kHalfArea)
            p.y += size;
        else if (p.y > kHalfArea)
            p.y -= size;
        if (p != before)
            body.SetPosition(p);
    }
}

void BroadPhaseScene::CountBruteForcePairs()
{
    struct Entry
    {
        size_t BodyIndex;
        bool IsStatic;
        Crumbs2D::AABB Box;
    };

    std::vector<Entry> entries;
    for (size_t b = 0; b < m_Bodies.size(); ++b)
        for (const Crumbs2D::Collider& collider : m_Bodies[b].GetColliders())
            entries.push_back({b, m_Bodies[b].IsStatic(), collider.GetAABB()});

    m_BruteForce = {};
    for (size_t i = 0; i < entries.size(); ++i)
        for (size_t j = i + 1; j < entries.size(); ++j)
        {
            if (!Crumbs2D::AABB::TestOverlap(entries[i].Box, entries[j].Box))
                continue;

            ++m_BruteForce.Overlapping;
            if (entries[i].BodyIndex == entries[j].BodyIndex)
                ++m_BruteForce.SameBody;
            else if (entries[i].IsStatic && entries[j].IsStatic)
                ++m_BruteForce.BothStatic;
            else
                ++m_BruteForce.Expected;
        }
}

void BroadPhaseScene::CollectTreeStats()
{
    m_Tree = {};
    m_World.ForEachBroadPhaseNode([this](const Crumbs2D::AABB& bound, uint16_t height, bool isLeaf) {
        ++m_Tree.NodeCount;
        m_Tree.Height = std::max<uint32_t>(m_Tree.Height, height);
        if (isLeaf)
            ++m_Tree.LeafCount;
        else
            m_Tree.TotalCost += Crumbs2D::AABB::Cost(bound);
    });
    m_Tree.IsValid = m_World.ValidateBroadPhase();
}

void BroadPhaseScene::OnDraw()
{
    // Arbre : nœuds internes par hauteur, puis AABB agrandies des feuilles.
    if (m_DrawTreeNodes || m_DrawFatAABBs)
    {
        const uint32_t maxHeight = m_Tree.Height;
        m_World.ForEachBroadPhaseNode([&](const Crumbs2D::AABB& bound, uint16_t height, bool isLeaf) {
            if (isLeaf)
            {
                if (m_DrawFatAABBs)
                    DrawAabb(bound, kFatAabbColor);
            }
            else if (m_DrawTreeNodes && height >= m_MinDrawnHeight)
            {
                DrawAabb(bound, HeightColor(height, maxHeight));
            }
        });
    }

    // Colliders et AABB exactes.
    for (const Crumbs2D::Body& body : m_Bodies)
    {
        const DrawColor color = body.IsStatic() ? kStaticColor : kColliderColor;

        for (const Crumbs2D::Collider& collider : body.GetColliders())
        {
            if (m_DrawExactAABBs)
                DrawAabb(collider.GetAABB(), kExactAabbColor);

            if (!m_DrawColliders)
                continue;

            const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();
            const glm::vec2 position = collider.GetGlobalPosition();

            switch (geometry.GetType())
            {
                case Crumbs2D::ShapeType::Circle:
                    RLRenderer::DrawCircle(position, geometry.AsCircle().Radius, color);
                    break;
                case Crumbs2D::ShapeType::Box:
                    RLRenderer::DrawBox(position, geometry.AsBox().HalfExtents, collider.GetGlobalRotation(), color);
                    break;
            }
        }
    }

    if (m_DrawPairLinks)
        for (const auto& [a, b] : m_Pairs)
            RLRenderer::DrawSegment(AabbCenter(a.GetAABB()), AabbCenter(b.GetAABB()), kLinkColor);
}

void BroadPhaseScene::DrawPairDiagnostics()
{
    const ImVec4 green(0.3f, 1.0f, 0.3f, 1.0f);
    const ImVec4 red(1.0f, 0.3f, 0.3f, 1.0f);
    const size_t found = m_Pairs.size();

    ImGui::Text("Paires renvoyees : %zu", found);
    ImGui::Text("Paires attendues : %zu", m_BruteForce.Expected);
    ImGui::Text(
        "  (chevauchements %zu, meme body %zu, statiques %zu)",
        m_BruteForce.Overlapping,
        m_BruteForce.SameBody,
        m_BruteForce.BothStatic
    );

    if (found == m_BruteForce.Expected)
        ImGui::TextColored(green, "OK : paires exactes");
    else if (found == m_BruteForce.Overlapping)
        ImGui::TextColored(red, "ERREUR : aucun filtre applique");
    else if (found == m_BruteForce.Expected + m_BruteForce.SameBody)
        ImGui::TextColored(red, "ERREUR : filtre 'meme body' manquant");
    else if (found == m_BruteForce.Expected + m_BruteForce.BothStatic)
        ImGui::TextColored(red, "ERREUR : filtre 'deux statiques' manquant");
    else if (found < m_BruteForce.Expected)
        ImGui::TextColored(red, "ERREUR : paires manquantes");
    else
        ImGui::TextColored(red, "ERREUR : paires en trop (AABB agrandies non filtrees ?)");
}

void BroadPhaseScene::DrawTreeDiagnostics()
{
    const ImVec4 green(0.3f, 1.0f, 0.3f, 1.0f);
    const ImVec4 red(1.0f, 0.3f, 0.3f, 1.0f);

    const double idealHeight = m_Tree.LeafCount > 1 ? std::ceil(std::log2(static_cast<double>(m_Tree.LeafCount))) : 0.0;

    ImGui::Text("Feuilles : %u   Noeuds : %u", m_Tree.LeafCount, m_Tree.NodeCount);
    ImGui::Text("Hauteur  : %u   (ideale : %.0f)", m_Tree.Height, idealHeight);
    ImGui::Text("Cout     : %.1f m", static_cast<double>(m_Tree.TotalCost));
    ImGui::TextColored(m_Tree.IsValid ? green : red, m_Tree.IsValid ? "Validate : OK" : "Validate : ECHEC");

    if (m_Tree.NodeCount != 0 && m_Tree.NodeCount != 2 * m_Tree.LeafCount - 1)
        ImGui::TextColored(red, "ERREUR : un arbre binaire de n feuilles doit avoir 2n - 1 noeuds");
}

void BroadPhaseScene::OnImGuiRender()
{
    ImGui::Begin("Broad phase");

    if (ImGui::CollapsingHeader("Paires", ImGuiTreeNodeFlags_DefaultOpen))
        DrawPairDiagnostics();

    if (ImGui::CollapsingHeader("Arbre", ImGuiTreeNodeFlags_DefaultOpen))
        DrawTreeDiagnostics();

    if (ImGui::CollapsingHeader("Affichage", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Colliders", &m_DrawColliders);
        ImGui::SameLine();
        ImGui::Checkbox("AABB exactes", &m_DrawExactAABBs);
        ImGui::Checkbox("AABB agrandies", &m_DrawFatAABBs);
        ImGui::SameLine();
        ImGui::Checkbox("Liens des paires", &m_DrawPairLinks);
        ImGui::Checkbox("Noeuds internes", &m_DrawTreeNodes);
        ImGui::SliderInt("Hauteur min.", &m_MinDrawnHeight, 1, std::max(1, static_cast<int>(m_Tree.Height)));
    }

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Clic gauche : deplacer la grande boite. Clic droit : deplacer la camera.");
        ImGui::Checkbox("Pause", &m_Paused);
        ImGui::SameLine();
        if (ImGui::Button("Pas a pas") && m_Paused)
        {
            m_World.Step();
            WrapBodies();
        }

        ImGui::InputInt("Nombre", &m_BodyCount, 10, 100);
        if (m_BodyCount < 0)
            m_BodyCount = 0;
        if (ImGui::Button("Reset"))
            CreateBodies();
    }

    ImGui::End();
}

void BroadPhaseScene::UpdateCameraNavigation()
{
    m_Camera.SetViewport(RLRenderer::GetViewportSize());

    if (RLInput::IsMouseCapturedByUI())
        return;

    if (RLInput::IsMouseButtonDown(InputMouseButton::Right))
        m_Camera.Pan(RLInput::GetMouseDelta());

    const float wheel = RLInput::GetMouseWheel();
    if (wheel != 0.0f)
        m_Camera.ZoomAt(RLInput::GetMousePosition(), std::pow(1.1f, wheel));
}