#include "Collider.h"

#include "ColliderStorage.h"
#include "CrumbsMath.h"
#include "World.h"
#include "Body.h"
#include "BodyStorage.h"
#include "BroadPhase.h"

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

glm::vec2 Collider::GetLocalCenter() const
{ return Transform::ApplyToPoint(GetLocalTransform(), ColliderGeometry::ComputeCenter(GetGeometry())); }

float Collider::GetFrictionCoefficient() const { return Value(&ColliderColumns::FrictionCoefficient); }

void Collider::SetFrictionCoefficient(float frictionCoefficient) const
{ Value(&ColliderColumns::FrictionCoefficient) = frictionCoefficient; }

const ColliderGeometry& Collider::GetGeometry() const { return Value(&ColliderColumns::Geometry); }

const AABB Collider::GetAABB() const { return Value(&ColliderColumns::AABB); }

Transform Collider::GetLocalTransform() const { return Value(&ColliderColumns::LocalTransform); }

void Collider::RefreshWorldPlacement() const
{
    const Transform worldTransform = Transform::Compose(GetBody().GetTransform(), GetLocalTransform());
    Value(&ColliderColumns::WorldTransform) = worldTransform;
    Value(&ColliderColumns::AABB) = ColliderGeometry::RecomputeAABB(GetGeometry(), worldTransform);
}

Transform Collider::GetWorldTransform() const { return Value(&ColliderColumns::WorldTransform); }
glm::vec2 Collider::GetWorldPosition() const { return GetWorldTransform().Position; }
float Collider::GetWorldRotation() const { return GetWorldTransform().Rotation; }

void Collider::SetLocalTransform(const Transform& localTransform) const
{
    Value(&ColliderColumns::LocalTransform) = localTransform;
    OnLocalTransformChanged();
}

void Collider::SetLocalPosition(glm::vec2 position) const
{
    Value(&ColliderColumns::LocalTransform).Position = position;
    OnLocalTransformChanged();
}

void Collider::SetLocalRotation(float rotation) const
{
    Value(&ColliderColumns::LocalTransform).Rotation = rotation;
    OnLocalTransformChanged();
}

void Collider::OnLocalTransformChanged() const
{
    GetBody().RecalculateMassData();
    RefreshWorldPlacement();
    m_World->m_BroadPhase->MoveProxy(Value(&ColliderColumns::Proxy), Value(&ColliderColumns::AABB));
}

} // namespace Crumbs2D