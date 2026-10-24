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

// Moyenne glissante : les valeurs restent lisibles malgré le bruit.
void Smooth(float& value, float sample) { value += 0.05f * (sample - value); }

constexpr glm::vec2 kDefaultGravity = {0.0f, -9.81f};

const DrawColor kColliderColor = {90, 170, 255, 255};
const DrawColor kAabbColor = {255, 200, 60, 255};

void Row(const char* label, float ms, float stepMs)
{
    const double share = stepMs > 0.0f ? 100.0 * static_cast<double>(ms / stepMs) : 0.0;
    ImGui::Text("%-14s %8.3f ms  %5.1f %%", label, static_cast<double>(ms), share);
}
} // namespace

void StressScene::OnInit()
{
    // Dézoomé pour voir toute la zone.
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
    std::uniform_real_distribution<float> position(-m_HalfArea, m_HalfArea);
    std::uniform_real_distribution<float> speed(-3.0f, 3.0f);
    std::uniform_real_distribution<float> spin(-2.0f, 2.0f);

    for (int i = 0; i < m_BodyCount; ++i)
    {
        Crumbs2D::Body body = m_World.CreateBody();
        body.SetPosition({position(rng), position(rng)});
        body.SetVelocity({speed(rng), speed(rng)});
        body.SetAngularVelocity(spin(rng));
        body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({{kHalfBox, kHalfBox}}));
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
    const float stepDuration = m_World.GetSolverSettings().DeltaTime;
    m_Accumulator += dt;

    int steps = 0;
    float wrapMs = 0.0f;
    while (m_Accumulator >= stepDuration && steps < kMaxStepsPerFrame)
    {
        m_World.Step();

        const Clock::time_point start = Clock::now();
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
        Smooth(m_WrapMs, wrapMs / static_cast<float>(steps));
}

void StressScene::WrapBodies()
{
    // Coût séparé du Step : passe par l'API publique, comme le ferait un jeu.
    const float size = 2.0f * m_HalfArea;

    for (const Crumbs2D::Body& body : m_Bodies)
    {
        glm::vec2 p = body.GetPosition();
        bool moved = false;

        if (p.x < -m_HalfArea)
        {
            p.x += size;
            moved = true;
        }
        else if (p.x > m_HalfArea)
        {
            p.x -= size;
            moved = true;
        }

        if (p.y < -m_HalfArea)
        {
            p.y += size;
            moved = true;
        }
        else if (p.y > m_HalfArea)
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
    if (!m_DrawColliders && !m_DrawAABBs)
        return;

    const Clock::time_point start = Clock::now();

    for (const Crumbs2D::Body& body : m_Bodies)
    {
        for (const Crumbs2D::Collider& collider : body.GetColliders())
        {
            if (m_DrawColliders)
            {
                const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();
                const glm::vec2 position = collider.GetWorldPosition();

                switch (geometry.GetType())
                {
                    case Crumbs2D::ShapeType::Circle:
                        RLRenderer::DrawCircle(position, geometry.AsCircle().Radius, kColliderColor);
                        break;
                    case Crumbs2D::ShapeType::Box:
                        RLRenderer::DrawBox(
                            position, geometry.AsBox().HalfExtents, collider.GetWorldRotation(), kColliderColor
                        );
                        break;
                }
            }

            if (m_DrawAABBs)
            {
                const Crumbs2D::AABB aabb = collider.GetAABB();
                RLRenderer::DrawBox((aabb.min + aabb.max) * 0.5f, (aabb.max - aabb.min) * 0.5f, 0.0f, kAabbColor);
            }
        }
    }

    // Ne mesure que l'envoi des commandes : raylib dessine réellement en fin d'image.
    Smooth(m_DrawMs, ElapsedMs(start));
}

void StressScene::OnImGuiRender()
{
    ImGui::Begin("Stress test");

    const double n = static_cast<double>(m_Bodies.size());
    ImGui::Text("Bodies           : %zu", m_Bodies.size());
    ImGui::Text("Tests d'AABB/pas : %.0f  (n(n-1)/2)", n * (n - 1.0) * 0.5);
    ImGui::Text("Paires trouvees  : %u", m_PairCount);
    ImGui::Separator();

    ImGui::Text(
        "Image : %.2f ms (%.0f FPS), %d pas",
        static_cast<double>(m_FrameMs),
        m_FrameMs > 0.0f ? 1000.0 / static_cast<double>(m_FrameMs) : 0.0,
        m_StepsLastFrame
    );
    ImGui::Separator();

    ImGui::Text("Scene :");
    ImGui::Text("%-14s %8.3f ms", "Wrap", static_cast<double>(m_WrapMs));
    ImGui::Text("%-14s %8.3f ms", "Dessin", static_cast<double>(m_DrawMs));
    ImGui::Separator();

    ImGui::Checkbox("Pause", &m_Paused);
    ImGui::SameLine();
    if (ImGui::Button("Pas a pas") && m_Paused)
    {
        m_World.Step();
        WrapBodies();
    }

    ImGui::Checkbox("Colliders", &m_DrawColliders);
    ImGui::SameLine();
    ImGui::Checkbox("AABB", &m_DrawAABBs);

    if (ImGui::Checkbox("Gravite", &m_UseGravity))
        m_World.PhysicsSettings.Gravity = m_UseGravity ? kDefaultGravity : glm::vec2(0.0f);

    ImGui::InputInt("Nombre", &m_BodyCount, 1000, 10000);
    if (m_BodyCount < 1)
        m_BodyCount = 1;
    ImGui::SliderFloat("Demi-zone (m)", &m_HalfArea, 5.0f, 200.0f, "%.0f");
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