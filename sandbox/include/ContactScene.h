#pragma once

#include "SandboxScene.h"
#include "SceneCamera.h"
#include "World.h"

#include <array>
#include <vector>
#include "RLRenderer.h"
#include "Crumbs2D.h"
class ContactScene : public SandboxScene
{
public:
    void OnInit() override;
    void OnUpdate(float dt) override;
    void OnDraw() override;
    void OnImGuiRender() override;

private:
    enum class ShapeChoice { Circle, Box };

    struct ShapeSettings
    {
        ShapeChoice Type = ShapeChoice::Box;
        float Radius = 1.0f;
        glm::vec2 HalfExtents = {1.5f, 0.75f};
        glm::vec2 Position = {0.0f, 0.0f};
        float RotationDegrees = 0.0f;
    };

    void RebuildWorld();
    void UpdateCameraNavigation();
    void UpdateDrag();
    void DrawShape(const Crumbs2D::Body& body, DrawColor color) const;

    SceneCamera m_Camera;
    Crumbs2D::World m_World;
    std::array<Crumbs2D::Body, 2> m_Bodies;
    std::array<ShapeSettings, 2> m_Settings;
    std::vector<Crumbs2D::ContactInfo> m_Contacts;

    int m_DraggedIndex = -1;
    glm::vec2 m_DragOffset = {0.0f, 0.0f};
    bool m_DrawMidSegments = true;
    float m_NormalLength = 0.75f;
};