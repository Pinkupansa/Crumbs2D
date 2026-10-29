#include "Collider.h"

#include "Body.h"
#include "ColliderStorage.h"
#include "World.h"
#include "WorldWrites.h"

#include <cassert>

namespace Crumbs2D
{
template <typename T> const T& Collider::Value(Column<T> ColliderColumns::* column) const
{
    assert(m_World != nullptr && "Collider is not attached to a world");
    return m_World->m_Colliders->Get({m_Handle, m_Generation}, column);
}

bool Collider::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }

Body Collider::GetBody() const
{
    const Id owner = Value(&ColliderColumns::Body);
    return Body(owner.Handle, owner.Generation, m_World);
}

Transform Collider::GetLocalTransform() const { return Value(&ColliderColumns::LocalTransform); }

void Collider::SetLocalTransform(const Transform& localTransform) const
{ m_World->SetColliderValue<&ColliderColumns::LocalTransform>({m_Handle, m_Generation}, localTransform); }

glm::vec2 Collider::GetLocalPosition() const { return GetLocalTransform().Position; }

void Collider::SetLocalPosition(glm::vec2 position) const
{
    Transform local = GetLocalTransform();
    local.Position = position;
    SetLocalTransform(local);
}

float Collider::GetLocalRotation() const { return GetLocalTransform().Rotation; }

void Collider::SetLocalRotation(float rotation) const
{
    Transform local = GetLocalTransform();
    local.Rotation = rotation;
    SetLocalTransform(local);
}

glm::vec2 Collider::GetLocalCenter() const
{ return Transform::ApplyToPoint(GetLocalTransform(), ColliderGeometry::ComputeCenter(GetGeometry())); }

Transform Collider::GetWorldTransform() const
{
    m_World->EnsureSynchronized();
    return Value(&ColliderColumns::WorldTransform);
}
glm::vec2 Collider::GetWorldPosition() const { return GetWorldTransform().Position; }
float Collider::GetWorldRotation() const { return GetWorldTransform().Rotation; }

AABB Collider::GetAABB() const
{
    m_World->EnsureSynchronized();
    return Value(&ColliderColumns::AABB);
}

float Collider::GetFrictionCoefficient() const { return Value(&ColliderColumns::FrictionCoefficient); }

void Collider::SetFrictionCoefficient(float frictionCoefficient) const
{ m_World->SetColliderValue<&ColliderColumns::FrictionCoefficient>({m_Handle, m_Generation}, frictionCoefficient); }

const ColliderGeometry& Collider::GetGeometry() const { return Value(&ColliderColumns::Geometry); }

void Collider::SetIsTrigger(bool trigger) const
{ m_World->SetColliderValue<&ColliderColumns::IsTrigger>({m_Handle, m_Generation}, trigger); }
} // namespace Crumbs2D