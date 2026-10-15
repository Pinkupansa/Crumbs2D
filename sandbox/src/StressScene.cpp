#include "StressScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <chrono>
#include <cmath>
#include <random>

namespace
{
using Clock = std::chrono::steady_clock;

float ElapsedMs(Clock::time_point start)
{ return std::chrono::duration<float, std::milli>(Clock::now() - start).count(); }

// Moyenne glissante : les valeurs affichées restent lisibles malgré le bruit.
void Smooth(float& value, float sample) { value += 0.05f * (sample - value); }

constexpr glm::vec2 kDefaultGravity = {0.0f, -9.81f};
} // namespace

void StressScene::OnInit()
{
    // Dézoomé pour voir toute la zone de 100 m de côté.
    m_Camera.SetZoom(6.0f);
    CreateBodies();
}

void StressScene::CreateBodies()
{
    m_World = Crumbs2D::World{Crumbs2D::SolverSettings{1.0f / 60.0f}};
    m_World.PhysicsSettings.Gravity = m_UseGravity ? kDefaultGravity : glm::vec2(0.0f);
    m_Bodies.clear();
    m_Bodies.reserve(static_cast<size_t>(m_BodyCount));

    // Graine fixe : la scène est identique à chaque reset, donc comparable.
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> position(-kHalfArea, kHalfArea);
    std::uniform_real_distribution<float> speed(-3.0f, 3.0f);
    std::uniform_real_distribution<float> spin(-2.0f, 2.0f);

    for (int i = 0; i < m_BodyCount; ++i)
    {
        Crumbs2D::Body body = m_World.CreateBody();
        body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{0.5f, 0.5f}}));
        body.SetPosition({position(rng), position(rng)});
        body.SetVelocity({speed(rng), speed(rng)});
        body.SetAngularVelocity(spin(rng));
        m_Bodies.push_back(body);
    }

    m_Accumulator = 0.0f;
}

void StressScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);

    Smooth(m_FrameMs, dt * 1000.0f);

    if (m_Paused)
        return;

    // Boucle à pas fixe, avec limite pour éviter la spirale de la mort.
    m_Accumulator += dt;
    int steps = 0;
    float stepMs = 0.0f;
    float wrapMs = 0.0f;

    const float stepDuration = m_World.GetSolverSettings().DeltaTime;
    while (m_Accumulator >= stepDuration && steps < kMaxStepsPerFrame)
    {
        Clock::time_point start = Clock::now();
        m_World.Step();
        stepMs += ElapsedMs(start);

        start = Clock::now();
        WrapBodies();
        wrapMs += ElapsedMs(start);

        m_Accumulator -= stepDuration;
        ++steps;
    }

    // Au-delà de la limite, on abandonne le retard plutôt que de le rattraper.
    if (steps == kMaxStepsPerFrame)
        m_Accumulator = 0.0f;

    m_StepsLastFrame = steps;
    if (steps > 0)
    {
        // Temps moyen d'UN pas, pour être indépendant du nombre de pas par image.
        Smooth(m_StepMs, stepMs / static_cast<float>(steps));
        Smooth(m_WrapMs, wrapMs / static_cast<float>(steps));
    }
}

void StressScene::WrapBodies()
{
    // Coût séparé du Step : passe par l'API publique, comme le ferait un jeu.
    constexpr float size = 2.0f * kHalfArea;

    for (const Crumbs2D::Body& body : m_Bodies)
    {
        glm::vec2 p = body.GetPosition();
        bool moved = false;

        if (p.x < -kHalfArea)
        {
            p.x += size;
            moved = true;
        }
        else if (p.x > kHalfArea)
        {
            p.x -= size;
            moved = true;
        }

        if (p.y < -kHalfArea)
        {
            p.y += size;
            moved = true;
        }
        else if (p.y > kHalfArea)
        {
            p.y -= size;
            moved = true;
        }

        if (moved)
            body.SetPosition(p);
    }
}

void StressScene::OnDraw()
{
    const Clock::time_point start = Clock::now();

    const DrawColor colliderColor = {90, 170, 255, 255};
    const DrawColor aabbColor = {255, 200, 60, 255};

    for (const Crumbs2D::Body& body : m_Bodies)
    {
        const glm::vec2 bodyPosition = body.GetPosition();
        const float bodyRotation = body.GetRotation();

        for (const Crumbs2D::Collider& collider : body.GetColliders())
        {
            if (m_DrawColliders)
            {
                const glm::vec2 position = bodyPosition + Crumbs2D::Rotate(collider.GetLocalPosition(), bodyRotation);
                const float rotation = bodyRotation + collider.GetLocalRotation();
                const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();

                switch (geometry.GetType())
                {
                    case Crumbs2D::ShapeType::Circle:
                        RLRenderer::DrawCircle(position, geometry.AsCircle().Radius, colliderColor);
                        break;
                    case Crumbs2D::ShapeType::Box:
                        RLRenderer::DrawBox(position, geometry.AsBox().HalfExtents, rotation, colliderColor);
                        break;
                }
            }

            if (m_DrawAABBs)
            {
                const Crumbs2D::AABB aabb = collider.GetAABB();
                RLRenderer::DrawBox((aabb.min + aabb.max) * 0.5f, (aabb.min - aabb.max) * 0.5f, 0.0f, aabbColor);
            }
        }
    }

    Smooth(m_DrawMs, ElapsedMs(start));
}

void StressScene::OnImGuiRender()
{
    ImGui::Begin("Stress test");

    ImGui::Text("Bodies : %zu", m_Bodies.size());
    ImGui::Text(
        "FPS    : %.0f (%.2f ms)",
        m_FrameMs > 0.0f ? 1000.0 / static_cast<double>(m_FrameMs) : 0.0,
        static_cast<double>(m_FrameMs)
    );
    ImGui::Separator();
    ImGui::Text("Step   : %.3f ms", static_cast<double>(m_StepMs));
    ImGui::Text("Wrap   : %.3f ms", static_cast<double>(m_WrapMs));
    ImGui::Text("Draw   : %.3f ms", static_cast<double>(m_DrawMs));
    ImGui::Text("Pas cette image : %d", m_StepsLastFrame);
    ImGui::Separator();

    ImGui::Checkbox("Pause", &m_Paused);
    ImGui::SameLine();
    if (ImGui::Button("Pas a pas") && m_Paused)
    {
        m_World.Step();
        WrapBodies();
    }

    // OnImGuiRender : remplace la case "Dessiner les boites"
    ImGui::Checkbox("Colliders", &m_DrawColliders);
    ImGui::SameLine();
    ImGui::Checkbox("AABB", &m_DrawAABBs);

    if (ImGui::Checkbox("Gravite", &m_UseGravity))
        m_World.PhysicsSettings.Gravity = m_UseGravity ? kDefaultGravity : glm::vec2(0.0f);

    ImGui::InputInt("Nombre", &m_BodyCount, 1000, 10000);
    if (m_BodyCount < 1)
        m_BodyCount = 1;
    if (ImGui::Button("Reset"))
        CreateBodies();

    ImGui::End();
}

void StressScene::UpdateCameraNavigation()
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