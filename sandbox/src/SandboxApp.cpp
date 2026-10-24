#include "SandboxApp.h"

#include "RLRenderer.h"
#include "RLWindow.h"
#define PROFILER_IMGUI_IMPLEMENTATION
#include "Profiler.h"
SandboxApp::SandboxApp(Crumbs2D::Scope<SandboxScene> scene) : m_Scene(std::move(scene)) {}

void SandboxApp::Run()
{
    RLWindow::Init(1280, 720, "Crumbs2D Sandbox", 0);

    m_Scene->OnInit();

    while (!RLWindow::ShouldClose())
    {
        const float dt = RLWindow::GetDeltaTime();

        m_Scene->OnUpdate(dt);

        RLRenderer::BeginFrame({30, 30, 34, 255});

        RLRenderer::BeginWorld();
        RLRenderer::DrawGrid();
        m_Scene->OnDraw();
        RLRenderer::EndWorld();

        RLRenderer::BeginImGui();
        m_Scene->OnImGuiRender();
#if PROFILER_ENABLED
        prof::DrawWindow();
#endif
        RLRenderer::EndImGui();

        RLRenderer::EndFrame();
    }

    RLWindow::Shutdown();
}