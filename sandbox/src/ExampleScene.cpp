#include "ExampleScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include "Geometry.h"
namespace
{
constexpr int kMaxStepsPerFrame = 4;
}

void ExampleScene::OnInit()
{
    m_BoxBody = m_World.CreateBody();
    m_BoxBody.SetPosition({0, 2});
    m_BoxBody.AddCollider(Crumbs2D::ColliderGeometry::MakeBox(Crumbs2D::BoxData({2, 1})));
    m_World.PhysicsSettings.Gravity = {0, -9.81f};
}

void ExampleScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);

    if (!RLInput::IsMouseCapturedByUI() && RLInput::IsMouseButtonPressed(InputMouseButton::Left))
    {
        const glm::vec2 mousePos = m_Camera.ScreenToWorld(RLInput::GetMousePosition());
        const glm::vec2 dir = m_BoxBody.GetPosition() - mousePos;
        const float distance = glm::length(dir);

        // Petite poussée aléatoire en s'éloignant du clic, si le clic n'est pas pile au centre.
        glm::vec2 push = {0.0f, 0.0f};
        if (distance > 1e-4f)
            push = dir / distance * ((float)rand() / (float)RAND_MAX) * 3.0f;

        m_BoxBody.AddImpulseAtPoint(glm::vec2{0, 10} + push, mousePos);
    }

    // Pas fixe : sa durée est celle des SolverSettings du world.
    const float fixedDeltaTime = m_World.GetSolverSettings().DeltaTime;
    m_SolverLag += dt;

    int steps = 0;
    while (m_SolverLag >= fixedDeltaTime && steps < kMaxStepsPerFrame)
    {
        m_World.Step();
        m_SolverLag -= fixedDeltaTime;
        ++steps;
    }

    // Au-delà de la limite, on abandonne le retard plutôt que de le rattraper.
    if (steps == kMaxStepsPerFrame)
        m_SolverLag = 0.0f;
}

void ExampleScene::OnDraw()
{
    Crumbs2D::Collider collider = m_BoxBody.GetCollider(0);
    RLRenderer::DrawBox(
        collider.GetGlobalPosition(),
        collider.GetGeometry().AsBox().HalfExtents,
        collider.GetGlobalRotation(),
        {0, 255, 0}
    );
    Crumbs2D::AABB aabb = collider.GetAABB();
    glm::vec2 middle = (aabb.min + aabb.max) / 2.0f;
    glm::vec2 extent = (aabb.max - aabb.min) / 2.0f;
    RLRenderer::DrawBox(middle, extent, 0, {255, 0, 0});
}

void ExampleScene::OnImGuiRender()
{
    const glm::vec2 mouse = m_Camera.ScreenToWorld(RLInput::GetMousePosition());
    const glm::vec2 position = m_Camera.GetPosition();
    float zoom = m_Camera.GetZoom();

    ImGui::Begin("Camera");
    ImGui::Text("Souris : (%.2f, %.2f) m", static_cast<double>(mouse.x), static_cast<double>(mouse.y));
    ImGui::Text("Cible  : (%.2f, %.2f) m", static_cast<double>(position.x), static_cast<double>(position.y));
    if (ImGui::SliderFloat(
            "Zoom (px/m)", &zoom, SceneCamera::kMinZoom, SceneCamera::kMaxZoom, "%.1f", ImGuiSliderFlags_Logarithmic
        ))
        m_Camera.SetZoom(zoom);
    if (ImGui::Button("Recentrer"))
        m_Camera.Reset();
    ImGui::End();
}

void ExampleScene::UpdateCameraNavigation()
{
    m_Camera.SetViewport(RLRenderer::GetViewportSize());

    if (RLInput::IsMouseCapturedByUI())
        return;

    if (RLInput::IsMouseButtonDown(InputMouseButton::Left))
        m_Camera.Pan(RLInput::GetMouseDelta());

    const float wheel = RLInput::GetMouseWheel();
    if (wheel != 0.0f)
        m_Camera.ZoomAt(RLInput::GetMousePosition(), std::pow(1.1f, wheel));
}