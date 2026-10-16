#include "BroadPhaseScene.h"

#include "CrumbsMath.h"
#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <cmath>
#include <random>

namespace
{
const DrawColor kColliderColor = {90, 170, 255, 255};
const DrawColor kStaticColor = {150, 150, 150, 255};
const DrawColor kAabbIdleColor = {80, 80, 90, 255};
const DrawColor kAabbPairColor = {255, 70, 70, 255};
const DrawColor kLinkColor = {255, 200, 60, 255};

glm::vec2 AabbCenter(const Crumbs2D::AABB& aabb) { return (aabb.min + aabb.max) * 0.5f; }
glm::vec2 AabbHalfSize(const Crumbs2D::AABB& aabb) { return (aabb.max - aabb.min) * 0.5f; }
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
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({size(rng)}));
        else
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{size(rng), size(rng) * 0.6f}}));

        m_Bodies.push_back(body);
    }

    // Piège 1 : un body à deux colliders qui se chevauchent -> aucune paire entre eux.
    Crumbs2D::Body compound = m_World.CreateBody();
    compound.SetPosition({-4.0f, 0.0f});
    compound.SetAngularVelocity(0.5f);
    compound.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.2f, 0.4f}}), {0.0f, 0.0f});
    compound.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({0.6f}), {1.0f, 0.0f});
    m_Bodies.push_back(compound);

    // Piège 2 : deux bodies statiques qui se chevauchent -> aucune paire entre eux.
    for (float x : {3.0f, 4.0f})
    {
        Crumbs2D::Body ground = m_World.CreateBody();
        ground.SetIsStatic(true);
        ground.SetPosition({x, -kHalfArea + 1.0f});
        ground.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.5f, 0.5f}}));
        m_Bodies.push_back(ground);
    }

    // Le body déplaçable à la souris : une grande boîte.
    m_DraggedBody = m_World.CreateBody();
    m_DraggedBody.SetPosition({0.0f, 0.0f});
    m_DraggedBody.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{1.5f, 1.0f}}));
    m_Bodies.push_back(m_DraggedBody);

    m_Accumulator = 0.0f;

    // Un premier pas pour avoir des AABB et des paires à afficher, même en pause.
    m_World.Step();
    m_Pairs = m_World.GetCandidatePairs();
    CountBruteForcePairs();
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
}

void BroadPhaseScene::UpdateDrag()
{
    if (RLInput::IsMouseCapturedByUI() || !RLInput::IsMouseButtonDown(InputMouseButton::Left))
        return;

    // Le body suit la souris, sans vitesse propre pour ne pas dériver.
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
    // Même règle que la broad phase, recalculée naïvement via l'API publique.
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

void BroadPhaseScene::OnDraw()
{
    for (const Crumbs2D::Body& body : m_Bodies)
    {
        const glm::vec2 bodyPosition = body.GetPosition();
        const float bodyRotation = body.GetRotation();
        const DrawColor color = body.IsStatic() ? kStaticColor : kColliderColor;

        for (const Crumbs2D::Collider& collider : body.GetColliders())
        {
            // AABB : rouge si le collider fait partie d'au moins une paire.
            bool inPair = false;
            for (const auto& [a, b] : m_Pairs)
                if (a == collider || b == collider)
                {
                    inPair = true;
                    break;
                }

            const Crumbs2D::AABB aabb = collider.GetAABB();
            RLRenderer::DrawBox(AabbCenter(aabb), AabbHalfSize(aabb), 0.0f, inPair ? kAabbPairColor : kAabbIdleColor);

            // Forme réelle du collider.
            const glm::vec2 position = bodyPosition + Crumbs2D::Rotate(collider.GetLocalPosition(), bodyRotation);
            const float rotation = bodyRotation + collider.GetLocalRotation();
            const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();

            switch (geometry.GetType())
            {
                case Crumbs2D::ShapeType::Circle:
                    RLRenderer::DrawCircle(position, geometry.AsCircle().Radius, color);
                    break;
                case Crumbs2D::ShapeType::Box:
                    RLRenderer::DrawBox(position, geometry.AsBox().HalfExtents, rotation, color);
                    break;
            }
        }
    }

    // Un segment entre les centres des AABB de chaque paire.
    if (m_DrawPairLinks)
        for (const auto& [a, b] : m_Pairs)
            RLRenderer::DrawSegment(AabbCenter(a.GetAABB()), AabbCenter(b.GetAABB()), kLinkColor);
}

void BroadPhaseScene::DrawDiagnostics()
{
    const ImVec4 green(0.3f, 1.0f, 0.3f, 1.0f);
    const ImVec4 red(1.0f, 0.3f, 0.3f, 1.0f);
    const size_t found = m_Pairs.size();

    ImGui::Text("Paires renvoyees par la broad phase : %zu", found);
    ImGui::Text("Paires attendues                    : %zu", m_BruteForce.Expected);
    ImGui::Separator();
    ImGui::Text("AABB qui se chevauchent (sans filtre) : %zu", m_BruteForce.Overlapping);
    ImGui::Text("  dont meme body                      : %zu", m_BruteForce.SameBody);
    ImGui::Text("  dont deux bodies statiques          : %zu", m_BruteForce.BothStatic);
    ImGui::Separator();

    if (found == m_BruteForce.Expected)
    {
        ImGui::TextColored(green, "OK : la broad phase renvoie exactement les paires attendues");
        return;
    }

    // Diagnostic : quel filtre manque ?
    if (found == m_BruteForce.Overlapping)
        ImGui::TextColored(red, "ERREUR : aucun filtre applique (meme body et statiques)");
    else if (found == m_BruteForce.Expected + m_BruteForce.SameBody)
        ImGui::TextColored(red, "ERREUR : le filtre 'meme body' manque");
    else if (found == m_BruteForce.Expected + m_BruteForce.BothStatic)
        ImGui::TextColored(red, "ERREUR : le filtre 'deux statiques' manque");
    else if (found < m_BruteForce.Expected)
        ImGui::TextColored(red, "ERREUR : des paires manquent (AABB pas a jour ?)");
    else
        ImGui::TextColored(red, "ERREUR : paires en trop ou en double");
}

void BroadPhaseScene::OnImGuiRender()
{
    ImGui::Begin("Broad phase");

    DrawDiagnostics();

    ImGui::Separator();
    ImGui::TextWrapped("Clic gauche : deplacer la grande boite. Clic droit : deplacer la camera.");
    ImGui::Checkbox("Pause", &m_Paused);
    ImGui::SameLine();
    if (ImGui::Button("Pas a pas") && m_Paused)
    {
        m_World.Step();
        WrapBodies();
    }
    ImGui::Checkbox("Liens entre paires", &m_DrawPairLinks);

    ImGui::InputInt("Nombre", &m_BodyCount, 10, 100);
    if (m_BodyCount < 0)
        m_BodyCount = 0;
    if (ImGui::Button("Reset"))
        CreateBodies();

    ImGui::End();
}

void BroadPhaseScene::UpdateCameraNavigation()
{
    m_Camera.SetViewport(RLRenderer::GetViewportSize());

    if (RLInput::IsMouseCapturedByUI())
        return;

    // Clic droit pour la caméra : le clic gauche sert à déplacer la boîte.
    if (RLInput::IsMouseButtonDown(InputMouseButton::Right))
        m_Camera.Pan(RLInput::GetMouseDelta());

    const float wheel = RLInput::GetMouseWheel();
    if (wheel != 0.0f)
        m_Camera.ZoomAt(RLInput::GetMousePosition(), std::pow(1.1f, wheel));
}