#include "ExampleScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <cmath>

void ExampleScene::OnInit()
{
    m_BoxBody = m_World.AddBody();
    m_BoxBody.SetPosition({0, 2});
}
void ExampleScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);

    if (RLInput::IsMouseButtonPressed(InputMouseButton::Left))
    {
        m_BoxBody.SetVelocity((m_BoxBody.GetPosition() - m_Camera.ScreenToWorld(RLInput::GetMousePosition())) * 2.0f);
        m_BoxBody.SetAngularVelocity(10);
    }
    m_World.Step(dt);
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