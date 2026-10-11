#include "ExampleScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <cmath>

static Crumbs2D::SolverSettings s_Settings = {1 / 60.0f};

void ExampleScene::OnInit()
{
    m_BoxBody = m_World.AddBody();
    m_BoxBody.SetPosition({0, 2});
    m_World.PhysicsSettings.Gravity = {0, -9.81f};
}
void ExampleScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);

    if (RLInput::IsMouseButtonPressed(InputMouseButton::Left))
    {
        glm::vec2 mousePos = m_Camera.ScreenToWorld(RLInput::GetMousePosition());
        glm::vec2 dir = m_BoxBody.GetPosition() - mousePos;
        m_BoxBody.AddImpulseAtPoint(glm::vec2{dir.x > 0 ? 1 : -1, dir.y > 0 ? 3 : -3} * 3.0f, mousePos);
    }

    m_SolverLag += dt;

    while (m_SolverLag > s_Settings.DeltaTime)
    {
        m_World.Step(s_Settings);
        m_SolverLag -= s_Settings.DeltaTime;
    }
}

void ExampleScene::OnDraw()
{ RLRenderer::DrawBox(m_BoxBody.GetPosition(), {2, 1}, m_BoxBody.GetRotation(), {0, 255, 0}); }

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