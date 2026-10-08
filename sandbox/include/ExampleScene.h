#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"

class ExampleScene : public SandboxScene
{
public:
    void OnUpdate(float dt) override;
    void OnDraw() override;
    void OnImGuiRender() override;

private:
    void UpdateCameraNavigation();

    SceneCamera m_Camera;
    float m_Angle = 0.0f;
};