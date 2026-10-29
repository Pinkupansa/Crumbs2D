#include "SolverScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <glm/geometric.hpp>

namespace
{
const DrawColor kDynamicColor = {90, 170, 255, 255};
const DrawColor kStaticColor = {150, 150, 150, 255};
const DrawColor kPenetratingColor = {255, 60, 60, 255};
const DrawColor kSpeculativeColor = {255, 220, 60, 255};
const DrawColor kNormalColor = {120, 230, 120, 255};

constexpr glm::vec2 kGravity = {0.0f, -9.81f};
} // namespace

void SolverScene::OnInit()
{
    m_Camera.SetZoom(40.0f);
    m_Camera.SetPosition({0.0f, 5.0f});
    RebuildWorld();
}

void SolverScene::RebuildWorld()
{
    Crumbs2D::SolverSettings settings;
    settings.DeltaTime = 1.0f / 60.0f;
    settings.Iterations = m_Iterations;
    settings.Beta = m_Beta;
    settings.Slop = m_Slop;

    m_World = Crumbs2D::World{settings};
    m_World.PhysicsSettings.Gravity = kGravity;
    m_Bodies.clear();
    m_Contacts.clear();
    m_Accumulator = 0.0f;

    CreateGround();

    switch (m_Preset)
    {
        case Preset::SingleBox:
            AddBox({0.0f, 3.0f}, {0.5f, 0.5f}, 0.3f);
            break;

        case Preset::Stack:
            // La référence du papier de Catto : des boîtes empilées, légèrement espacées.
            for (int i = 0; i < m_StackHeight; ++i)
                AddBox({0.0f, 0.5f + 1.05f * static_cast<float>(i)}, {0.5f, 0.5f});
            break;

        case Preset::Pyramid: {
            const int base = m_StackHeight;
            for (int row = 0; row < base; ++row)
                for (int i = 0; i < base - row; ++i)
                {
                    const float x = (static_cast<float>(i) - 0.5f * static_cast<float>(base - row - 1)) * 1.05f;
                    AddBox({x, 0.5f + 1.05f * static_cast<float>(row)}, {0.5f, 0.5f});
                }
            break;
        }

        case Preset::Mixed: {
            std::mt19937 rng(3);
            std::uniform_real_distribution<float> x(-4.0f, 4.0f);
            std::uniform_real_distribution<float> size(0.25f, 0.7f);
            std::uniform_real_distribution<float> angle(0.0f, 3.1415926f);
            for (int i = 0; i < 40; ++i)
            {
                const glm::vec2 position = {x(rng), 2.0f + 0.9f * static_cast<float>(i)};
                if (i % 2 == 0)
                    AddCircle(position, size(rng));
                else
                    AddBox(position, {size(rng), size(rng)}, angle(rng));
            }
            break;
        }
    }

    CollectStats();
}

void SolverScene::CreateGround()
{
    // Un sol large et deux murs, tous statiques.
    Crumbs2D::Body ground = m_World.CreateBody();
    ground.SetIsStatic(true);
    ground.SetPosition({0.0f, -0.5f});
    ground.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{12.0f, 0.5f}}));
    m_Bodies.push_back(ground);

    for (float x : {-12.5f, 12.5f})
    {
        Crumbs2D::Body wall = m_World.CreateBody();
        wall.SetIsStatic(true);
        wall.SetPosition({x, 5.0f});
        wall.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{0.5f, 6.0f}}));
        m_Bodies.push_back(wall);
    }
}

Crumbs2D::Body SolverScene::AddBox(glm::vec2 position, glm::vec2 halfExtents, float rotation)
{
    Crumbs2D::Body body = m_World.CreateBody();
    body.SetPosition(position);
    body.SetRotation(rotation);
    body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({halfExtents}));
    m_Bodies.push_back(body);
    return body;
}

Crumbs2D::Body SolverScene::AddCircle(glm::vec2 position, float radius)
{
    Crumbs2D::Body body = m_World.CreateBody();
    body.SetPosition(position);
    body.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({radius}));
    m_Bodies.push_back(body);
    return body;
}

void SolverScene::SpawnAtMouse()
{
    const glm::vec2 mouse = m_Camera.ScreenToWorld(RLInput::GetMousePosition());
    Crumbs2D::Body body;
    if (m_SpawnCircles)
        body = AddCircle(mouse, 0.4f);
    else
        body = AddBox(mouse, {0.5f, 0.5f});
    body.GetCollider(0).SetIsTrigger(m_SpawnTrigger);
    body.SetIsStatic(m_SpawnStatic);
}

void SolverScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);

    if (!RLInput::IsMouseCapturedByUI() && RLInput::IsMouseButtonPressed(InputMouseButton::Left))
        SpawnAtMouse();

    if (!m_Paused)
    {
        const float stepDuration = m_World.GetSolverSettings().DeltaTime;
        m_Accumulator += dt;

        int steps = 0;
        while (m_Accumulator >= stepDuration && steps < kMaxStepsPerFrame)
        {
            m_World.Step();
            m_Accumulator -= stepDuration;
            ++steps;
        }
        if (steps == kMaxStepsPerFrame)
            m_Accumulator = 0.0f;
    }

    m_Contacts = m_World.GetContacts();
    CollectStats();
}

void SolverScene::CollectStats()
{
    m_PointCount = 0;
    m_MaxPenetration = 0.0f;
    for (const Crumbs2D::ContactInfo& contact : m_Contacts)
    {
        m_PointCount += contact.PointCount;
        for (uint32_t i = 0; i < contact.PointCount; ++i)
            m_MaxPenetration = std::max(m_MaxPenetration, -contact.Separations[i]);
    }

    // Vitesse maximale : une pile au repos doit tendre vers zéro.
    m_MaxSpeed = 0.0f;
    for (const Crumbs2D::Body& body : m_Bodies)
        if (!body.IsStatic())
            m_MaxSpeed = std::max(m_MaxSpeed, glm::length(body.GetVelocity()));
}

void SolverScene::OnDraw()
{
    for (const Crumbs2D::Body& body : m_Bodies)
    {
        const DrawColor color = body.IsStatic() ? kStaticColor : kDynamicColor;

        for (const Crumbs2D::Collider& collider : body.GetColliders())
        {
            const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();
            const Crumbs2D::Transform xf = collider.GetWorldTransform();

            switch (geometry.GetType())
            {
                case Crumbs2D::ShapeType::Circle:
                    RLRenderer::DrawCircle(xf.Position, geometry.AsCircle().Radius, color);
                    // Un rayon pour voir la rotation.
                    RLRenderer::DrawSegment(xf.Position, xf.Position + geometry.AsCircle().Radius * xf.Right(), color);
                    break;
                case Crumbs2D::ShapeType::Box:
                    RLRenderer::DrawBox(xf.Position, geometry.AsBox().HalfExtents, xf.Rotation, color);
                    break;
            }
        }
    }

    if (!m_DrawContacts)
        return;

    const float markerRadius = 3.0f / m_Camera.GetZoom();
    for (const Crumbs2D::ContactInfo& contact : m_Contacts)
        for (uint32_t i = 0; i < contact.PointCount; ++i)
        {
            const glm::vec2 point = contact.Points[i];
            const DrawColor color = contact.Separations[i] <= 0.0f ? kPenetratingColor : kSpeculativeColor;
            RLRenderer::DrawCircle(point, markerRadius, color);
            RLRenderer::DrawSegment(point, point + 0.3f * contact.Normal, kNormalColor);
        }
}

void SolverScene::OnImGuiRender()
{
    ImGui::Begin("Solveur");

    ImGui::SeparatorText("Etat");
    ImGui::Text("Bodies : %zu   Contacts : %zu   Points : %zu", m_Bodies.size(), m_Contacts.size(), m_PointCount);
    ImGui::Text("Penetration max : %.4f m", static_cast<double>(m_MaxPenetration));
    ImGui::Text("Vitesse max     : %.4f m/s", static_cast<double>(m_MaxSpeed));

    ImGui::SeparatorText("Simulation");
    ImGui::Checkbox("Pause", &m_Paused);
    ImGui::SameLine();
    if (ImGui::Button("Pas a pas") && m_Paused)
        m_World.Step();
    ImGui::Checkbox("Afficher les contacts", &m_DrawContacts);
    ImGui::Text("Propriétés du prochain body");
    ImGui::Checkbox("Cercle (sinon boite)", &m_SpawnCircles);
    ImGui::Checkbox("Trigger", &m_SpawnTrigger);
    ImGui::Checkbox("Static", &m_SpawnStatic);
    ImGui::SeparatorText("Scene (appliquee au reset)");
    const char* presetNames[] = {"Une boite", "Pile", "Pyramide", "Melange"};
    int preset = static_cast<int>(m_Preset);
    if (ImGui::Combo("Preset", &preset, presetNames, 4))
    {
        m_Preset = static_cast<Preset>(preset);
        RebuildWorld();
    }
    ImGui::SliderInt("Hauteur / base", &m_StackHeight, 1, 20);

    ImGui::SeparatorText("Solveur (applique au reset)");
    ImGui::SliderInt("Iterations", &m_Iterations, 1, 50);
    ImGui::SliderFloat("Beta", &m_Beta, 0.0f, 1.0f);
    ImGui::SliderFloat("Slop (m)", &m_Slop, 0.0f, 0.05f, "%.3f");

    if (ImGui::Button("Reset"))
        RebuildWorld();

    ImGui::SeparatorText("Aide");
    ImGui::TextWrapped(
        "Clic gauche : ajouter une forme. Clic droit : camera. Molette : zoom. "
        "Une pile au repos doit garder une vitesse max proche de zero et une penetration "
        "de l'ordre du slop."
    );

    ImGui::End();
}

void SolverScene::UpdateCameraNavigation()
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