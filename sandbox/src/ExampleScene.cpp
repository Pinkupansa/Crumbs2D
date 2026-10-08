#include "ExampleScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <cmath>

void ExampleScene::OnUpdate(float dt)
{
    UpdateCameraNavigation();

    // La caméra doit être transmise avant BeginWorld(), appelé par SandboxApp.
    RLRenderer::SetCamera(m_Camera);

    m_Angle += dt;
}

void ExampleScene::OnDraw()
{
    RLRenderer::DrawCircle({2.0f, 1.0f}, 1.0f, {255, 161, 0, 255});
    RLRenderer::DrawBox({-3.0f, 2.0f}, {1.0f, 0.5f}, m_Angle, {0, 121, 241, 255});
}

void ExampleScene::OnImGuiRender()
{
    const glm::vec2 mouse = m_Camera.ScreenToWorld(RLInput::GetMousePosition());
    const glm::vec2 position = m_Camera.GetPosition();
    float zoom = m_Camera.GetZoom();

    ImGui::Begin("Camera");
    ImGui::Text("Souris : (%.2f, %.2f) m", static_cast<double>(mouse.x), static_cast<double>(mouse.y));
    ImGui::Text("Cible  : (%.2f, %.2f) m", static_cast<double>(position.x), static_cast<double>(position.y));
    if (ImGui::SliderFloat("Zoom (px/m)", &zoom, SceneCamera::kMinZoom, SceneCamera::kMaxZoom,
                           "%.1f", ImGuiSliderFlags_Logarithmic))
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