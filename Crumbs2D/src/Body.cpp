#include "Body.h"
#include "World.h"

// Les définitions sont ici et non dans le header : World.h inclut Body.h,
// donc Body.h ne peut voir que la déclaration avancée de World.

namespace Crumbs2D
{
bool Body::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }

void Body::SetVelocity(glm::vec2 velocity) const { m_World->Field(*this, &World::m_Velocities) = velocity; }
glm::vec2 Body::GetVelocity() const { return m_World->Field(*this, &World::m_Velocities); }

void Body::SetPosition(glm::vec2 position) const { m_World->Field(*this, &World::m_Positions) = position; }
glm::vec2 Body::GetPosition() const { return m_World->Field(*this, &World::m_Positions); }

void Body::SetAngularVelocity(float angularVelocity) const
{ m_World->Field(*this, &World::m_AngularVelocities) = angularVelocity; }
float Body::GetAngularVelocity() const { return m_World->Field(*this, &World::m_AngularVelocities); }

void Body::SetRotation(float rotation) const { m_World->Field(*this, &World::m_Rotations) = rotation; }
float Body::GetRotation() const { return m_World->Field(*this, &World::m_Rotations); }

void Body::SetMass(float mass) const { m_World->SetMass(*this, mass); }
float Body::GetMass() const { return m_World->GetMass(*this); }

void Body::SetLinearDrag(float linearDrag) const { m_World->Field(*this, &World::m_LinearDrags) = linearDrag; }
float Body::GetLinearDrag() const { return m_World->Field(*this, &World::m_LinearDrags); }

void Body::SetAngularDrag(float angularDrag) const { m_World->Field(*this, &World::m_AngularDrags) = angularDrag; }
float Body::GetAngularDrag() const { return m_World->Field(*this, &World::m_AngularDrags); }

void Body::SetIsStatic(bool isStatic) const
{
    m_World->Field(*this, &World::m_IsStatic) = isStatic;
    m_World->Field(*this, &World::m_InverseMasses) = 0;
    m_World->Field(*this, &World::m_InverseInertias) = 0;
}
bool Body::IsStatic() const { return m_World->Field(*this, &World::m_IsStatic) != 0; }

void Body::AddForce(glm::vec2 force) const { m_World->AddForce(*this, force); }

void Body::AddTorque(float torque) const { m_World->AddTorque(*this, torque); }

} // namespace Crumbs2D