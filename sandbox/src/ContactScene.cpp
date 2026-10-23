#include "ContactScene.h"

#include "RLInput.h"
#include "RLRenderer.h"

#include "imgui.h"

#include <cmath>
#include "glm/geometric.hpp"

namespace
{
const DrawColor kFirstColor = {90, 170, 255, 255};
const DrawColor kSecondColor = {255, 150, 80, 255};
const DrawColor kPenetratingColor = {255, 60, 60, 255};
const DrawColor kSpeculativeColor = {255, 220, 60, 255};
const DrawColor kNormalColor = {120, 230, 120, 255};

constexpr float kDegreesToRadians = 3.14159265358979323846f / 180.0f;
} // namespace

void ContactScene::OnInit()
{
    m_Camera.SetZoom(60.0f);

    m_Settings[0].Type = ShapeChoice::Box;
    m_Settings[0].HalfExtents = {2.0f, 0.5f};
    m_Settings[0].Position = {0.0f, -1.0f};

    m_Settings[1].Type = ShapeChoice::Box;
    m_Settings[1].HalfExtents = {0.75f, 0.75f};
    m_Settings[1].Position = {0.5f, 0.4f};
    m_Settings[1].RotationDegrees = 20.0f;

    RebuildWorld();
}

void ContactScene::RebuildWorld()
{
    m_World = Crumbs2D::World{Crumbs2D::SolverSettings{1.0f / 60.0f}};
    m_World.PhysicsSettings.Gravity = {0.0f, 0.0f};

    for (size_t i = 0; i < 2; ++i)
    {
        const ShapeSettings& settings = m_Settings[i];
        Crumbs2D::Body body = m_World.CreateBody();
        body.SetPosition(settings.Position);
        body.SetRotation(settings.RotationDegrees * kDegreesToRadians);

        if (settings.Type == ShapeChoice::Circle)
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeCircle({settings.Radius}));
        else
            body.AddCollider(Crumbs2D::ColliderGeometry::MakeBox({settings.HalfExtents}));

        m_Bodies[i] = body;
    }
}

void ContactScene::OnUpdate(float)
{
    UpdateCameraNavigation();
    RLRenderer::SetCamera(m_Camera);
    UpdateDrag();

    // Les deux bodies sont dynamiques, sans gravité ni vitesse : un Step ne les déplace pas,
    // mais recalcule les AABB, les paires et les contacts.
    for (size_t i = 0; i < 2; ++i)
    {
        m_Bodies[i].SetPosition(m_Settings[i].Position);
        m_Bodies[i].SetRotation(m_Settings[i].RotationDegrees * kDegreesToRadians);
        m_Bodies[i].SetVelocity({0.0f, 0.0f});
        m_Bodies[i].SetAngularVelocity(0.0f);
    }
    m_World.Step();

    m_Contacts = m_World.GetContacts();
}

void ContactScene::UpdateDrag()
{
    if (RLInput::IsMouseCapturedByUI())
    {
        m_DraggedIndex = -1;
        return;
    }

    const glm::vec2 mouse = m_Camera.ScreenToWorld(RLInput::GetMousePosition());

    if (!RLInput::IsMouseButtonDown(InputMouseButton::Left))
    {
        m_DraggedIndex = -1;
        return;
    }

    // Au début du glissement, on choisit la forme dont le centre est le plus proche.
    if (m_DraggedIndex < 0)
    {
        const float distance0 = glm::length(mouse - m_Settings[0].Position);
        const float distance1 = glm::length(mouse - m_Settings[1].Position);
        m_DraggedIndex = distance0 <= distance1 ? 0 : 1;
        m_DragOffset = m_Settings[m_DraggedIndex].Position - mouse;
    }

    m_Settings[m_DraggedIndex].Position = mouse + m_DragOffset;
}

void ContactScene::DrawShape(const Crumbs2D::Body& body, DrawColor color) const
{
    for (const Crumbs2D::Collider& collider : body.GetColliders())
    {
        const Crumbs2D::ColliderGeometry& geometry = collider.GetGeometry();
        const Crumbs2D::Transform xf = collider.GetWorldTransform();

        switch (geometry.GetType())
        {
            case Crumbs2D::ShapeType::Circle:
                RLRenderer::DrawCircle(xf.Position, geometry.AsCircle().Radius, color);
                RLRenderer::DrawSegment(xf.Position, xf.Position + geometry.AsCircle().Radius * xf.Right(), color);
                break;
            case Crumbs2D::ShapeType::Box:
                RLRenderer::DrawBox(xf.Position, geometry.AsBox().HalfExtents, xf.Rotation, color);
                break;
        }
    }
}

void ContactScene::OnDraw()
{
    DrawShape(m_Bodies[0], kFirstColor);
    DrawShape(m_Bodies[1], kSecondColor);

    const float markerRadius = 4.0f / m_Camera.GetZoom();

    for (const Crumbs2D::ContactInfo& contact : m_Contacts)
    {
        for (uint32_t i = 0; i < contact.PointCount; ++i)
        {
            const glm::vec2 point = contact.Points[i];
            const float separation = contact.Separations[i];
            const DrawColor color = separation <= 0.0f ? kPenetratingColor : kSpeculativeColor;

            RLRenderer::DrawCircle(point, markerRadius, color);
            RLRenderer::DrawSegment(point, point + m_NormalLength * contact.Normal, kNormalColor);

            // Le segment entre les deux surfaces : de longueur |séparation|, centré sur le point.
            if (m_DrawMidSegments)
            {
                const glm::vec2 halfGap = 0.5f * separation * contact.Normal;
                RLRenderer::DrawSegment(point - halfGap, point + halfGap, color);
            }
        }
    }
}

void ContactScene::OnImGuiRender()
{
    ImGui::Begin("Contacts");

    const char* typeNames[] = {"Cercle", "Boite"};
    bool rebuild = false;

    for (size_t i = 0; i < 2; ++i)
    {
        ShapeSettings& settings = m_Settings[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::SeparatorText(i == 0 ? "Forme A (bleue)" : "Forme B (orange)");

        int type = static_cast<int>(settings.Type);
        if (ImGui::Combo("Type", &type, typeNames, 2))
        {
            settings.Type = static_cast<ShapeChoice>(type);
            rebuild = true;
        }

        if (settings.Type == ShapeChoice::Circle)
            rebuild |= ImGui::SliderFloat("Rayon", &settings.Radius, 0.1f, 3.0f);
        else
            rebuild |= ImGui::SliderFloat2("Demi-dimensions", &settings.HalfExtents.x, 0.1f, 3.0f);

        ImGui::SliderFloat("Rotation (deg)", &settings.RotationDegrees, -180.0f, 180.0f);
        ImGui::DragFloat2("Position", &settings.Position.x, 0.01f);
        ImGui::PopID();
    }

    if (rebuild)
        RebuildWorld();

    ImGui::SeparatorText("Affichage");
    ImGui::Checkbox("Ecart entre les surfaces", &m_DrawMidSegments);
    ImGui::SliderFloat("Longueur des normales", &m_NormalLength, 0.1f, 2.0f);

    ImGui::SeparatorText("Resultat");
    if (m_Contacts.empty())
        ImGui::TextUnformatted("Aucun contact");

    for (const Crumbs2D::ContactInfo& contact : m_Contacts)
    {
        ImGui::Text(
            "Normale (A -> B) : (%.3f, %.3f)",
            static_cast<double>(contact.Normal.x),
            static_cast<double>(contact.Normal.y)
        );
        ImGui::Text("Points : %u", contact.PointCount);
        for (uint32_t i = 0; i < contact.PointCount; ++i)
        {
            ImGui::Text(
                "  [%u] sep %+.4f  id %08X  (%.3f, %.3f)",
                i,
                static_cast<double>(contact.Separations[i]),
                contact.FeatureIDs[i],
                static_cast<double>(contact.Points[i].x),
                static_cast<double>(contact.Points[i].y)
            );
        }
    }

    ImGui::SeparatorText("Aide");
    ImGui::TextWrapped(
        "Clic gauche : deplacer la forme la plus proche. Clic droit : camera. "
        "Rouge : penetration. Jaune : contact speculatif."
    );

    ImGui::End();
}

void ContactScene::UpdateCameraNavigation()
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