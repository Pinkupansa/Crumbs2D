#include "Collider.h"
#include "ColliderStorage.h"
#include "CrumbsMath.h"
#include "World.h"
#include "Body.h"
#include "BodyStorage.h"
#include <cassert>

namespace Crumbs2D
{
template <typename T> T& Collider::Value(Column<T> ColliderColumns::* column) const
{
    assert(m_World != nullptr && "Collider is not attached to a world !");
    return m_World->m_Colliders->Get({m_Handle, m_Generation}, column);
}

bool Collider::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }
Body Collider::GetBody() const
{
    Id ownerId = Value(&ColliderColumns::Body);
    return Body(ownerId.Handle, ownerId.Generation, m_World);
}

glm::vec2 Collider::GetLocalPosition() const { return Value(&ColliderColumns::LocalPosition); }

void Collider::SetLocalPosition(glm::vec2 position) const
{
    Value(&ColliderColumns::LocalPosition) = position;
    GetBody().RecalculateMassData();
    RecomputeAABB();
}

// Collider.cpp
glm::vec2 Collider::GetGlobalPosition() const
{ return GetBody().GetPosition() + Rotate(GetLocalPosition(), GetBody().GetRotation()); }

float Collider::GetFrictionCoefficient() const { return Value(&ColliderColumns::FrictionCoefficient); }

void Collider::RecomputeAABB() const
{
    Value(&ColliderColumns::AABB) =
        ColliderGeometry::RecomputeAABB(GetGeometry(), GetGlobalPosition(), GetGlobalRotation());
}
float Collider::GetGlobalRotation() const { return GetBody().GetRotation() + GetLocalRotation(); }

float Collider::GetLocalRotation() const { return Value(&ColliderColumns::LocalRotation); }

void Collider::SetLocalRotation(float rotation) const
{
    Value(&ColliderColumns::LocalRotation) = rotation;
    RecomputeAABB();
}

void Collider::SetFrictionCoefficient(float frictionCoefficient) const
{ Value(&ColliderColumns::FrictionCoefficient) = frictionCoefficient; }

const ColliderGeometry& Collider::GetGeometry() const { return Value(&ColliderColumns::Geometry); }

const AABB Collider::GetAABB() const { return Value(&ColliderColumns::AABB); }

} // namespace Crumbs2D