#include "Body.h"
#include "World.h"

// Les définitions sont ici et non dans le header : World.h inclut Body.h,
// donc Body.h ne peut voir que la déclaration avancée de World.

namespace Crumbs2D
{
bool Body::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }

void Body::SetVelocity(glm::vec2 velocity) const { m_World->SetVelocity(*this, velocity); }
glm::vec2 Body::GetVelocity() const { return m_World->GetVelocity(*this); }

void Body::SetPosition(glm::vec2 position) const { m_World->SetPosition(*this, position); }
glm::vec2 Body::GetPosition() const { return m_World->GetPosition(*this); }

void Body::SetAngularVelocity(float angularVelocity) const { m_World->SetAngularVelocity(*this, angularVelocity); }
float Body::GetAngularVelocity() const { return m_World->GetAngularVelocity(*this); }

void Body::SetRotation(float rotation) const { m_World->SetRotation(*this, rotation); }
float Body::GetRotation() const { return m_World->GetRotation(*this); }

void Body::SetMass(float mass) const { m_World->SetMass(*this, mass); }
float Body::GetMass() const { return m_World->GetMass(*this); }

void Body::AddForce(glm::vec2 force) const { m_World->AddForce(*this, force); }

void Body::AddTorque(float torque) const { m_World->AddTorque(*this, torque); }

} // namespace Crumbs2D