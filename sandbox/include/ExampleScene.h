#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "Crumbs2D.h"

class ExampleScene : public SandboxScene
{
public:
    void OnUpdate(float dt) override;
    void OnDraw() override;
    void OnImGuiRender() override;
    void OnInit() override;

private:
    void UpdateCameraNavigation();

    SceneCamera m_Camera;
    float m_Angle = 0.0f;

    Crumbs2D::World m_World;
    Crumbs2D::Body m_BoxBody;
};