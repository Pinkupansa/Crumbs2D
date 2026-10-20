#include "Body.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "CrumbsMath.h"
#include "World.h"
#include "glm/geometric.hpp"
#include "BroadPhase.h"
#include <cassert>

namespace Crumbs2D
{
template <typename T> T& Body::Value(Column<T> BodyColumns::* column) const
{
    assert(m_World != nullptr && "Body is not attached to a world !");
    return m_World->m_Bodies->Get({m_Handle, m_Generation}, column);
}

void Body::RecalculateMassData() const
{

    float totalArea = 0;
    glm::vec2 weightedCenter = {0.0f, 0.0f};

    for (const Collider& coll : GetColliders())
    {
        const float area = ColliderGeometry::ComputeArea(coll.GetGeometry());
        totalArea += area;
        weightedCenter += area * coll.GetLocalPosition();
    }

    if (totalArea == 0)
    {
        Value(&BodyColumns::MassCenter) = Value(&BodyColumns::Position);
        Value(&BodyColumns::InverseInertia) = 0;
        Value(&BodyColumns::MassCenterLocal) = glm::vec2(0);
        return;
    }

    const glm::vec2 massCenter = weightedCenter / totalArea;

    float density = GetMass() / totalArea;
    float totalInertia = 0;
    for (const Collider& coll : GetColliders())
    {
        const ColliderGeometry& geometry = coll.GetGeometry();
        float colliderMass = density * ColliderGeometry::ComputeArea(geometry);

        const glm::vec2 offset = coll.GetLocalPosition() - massCenter;
        totalInertia += density * ColliderGeometry::ComputeUnitInertia(geometry) // autour de son propre centre
                        + colliderMass * glm::dot(offset, offset);               // décalage (Huygens)
    }

    Value(&BodyColumns::MassCenterLocal) = massCenter;
    Value(&BodyColumns::MassCenter) = Value(&BodyColumns::Position) + Rotate(massCenter, Value(&BodyColumns::Rotation));
    Value(&BodyColumns::InverseInertia) = (!IsStatic() && totalInertia > 0.0f) ? 1.0f / totalInertia : 0.0f;
}

bool Body::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }

void Body::SetVelocity(glm::vec2 velocity) const { Value(&BodyColumns::Velocity) = velocity; }
glm::vec2 Body::GetVelocity() const { return Value(&BodyColumns::Velocity); }

void Body::SetPosition(glm::vec2 position) const
{
    Value(&BodyColumns::Position) = position;
    Value(&BodyColumns::MassCenter) =
        position + Rotate(Value(&BodyColumns::MassCenterLocal), Value(&BodyColumns::Rotation));
}
glm::vec2 Body::GetPosition() const { return Value(&BodyColumns::Position); }

void Body::SetAngularVelocity(float angularVelocity) const { Value(&BodyColumns::AngularVelocity) = angularVelocity; }
float Body::GetAngularVelocity() const { return Value(&BodyColumns::AngularVelocity); }

void Body::SetRotation(float rotation) const
{
    Value(&BodyColumns::Rotation) = rotation;
    Value(&BodyColumns::MassCenter) =
        Value(&BodyColumns::Position) + Rotate(Value(&BodyColumns::MassCenterLocal), Value(&BodyColumns::Rotation));
}
float Body::GetRotation() const { return Value(&BodyColumns::Rotation); }

void Body::SetMass(float mass) const
{
    // Une masse nulle ou négative donne une masse inverse nulle : body immobile.
    Value(&BodyColumns::InverseMass) = mass > 0.0f ? 1.0f / mass : 0.0f;
    RecalculateMassData();
}

float Body::GetMass() const
{
    const float inverseMass = Value(&BodyColumns::InverseMass);
    return inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
}

void Body::SetIsStatic(bool isStatic) const
{
    Value(&BodyColumns::IsStatic) = isStatic;
    Value(&BodyColumns::InverseInertia) = 0;
    Value(&BodyColumns::InverseMass) = 0;
}
bool Body::IsStatic() const { return Value(&BodyColumns::IsStatic) != 0; }

void Body::AddForce(glm::vec2 force) const { Value(&BodyColumns::Force) += force; }
void Body::AddTorque(float torque) const { Value(&BodyColumns::Torque) += torque; }

void Body::AddImpulse(glm::vec2 impulse) const
{ Value(&BodyColumns::Velocity) += impulse * Value(&BodyColumns::InverseMass); }

void Body::AddAngularImpulse(float impulse) const
{ Value(&BodyColumns::AngularVelocity) += impulse * Value(&BodyColumns::InverseInertia); }

void Body::AddForceAtPoint(glm::vec2 force, glm::vec2 point) const
{
    const glm::vec2 leverArm = point - Value(&BodyColumns::MassCenter);
    AddTorque(Cross(leverArm, force));
    AddForce(force);
}

void Body::AddImpulseAtPoint(glm::vec2 impulse, glm::vec2 point) const
{
    const glm::vec2 leverArm = point - Value(&BodyColumns::MassCenter);
    AddAngularImpulse(Cross(leverArm, impulse));
    AddImpulse(impulse);
}

uint32_t Body::GetColliderCount() const { return Value(&BodyColumns::ColliderCount); }

Collider Body::AddCollider(const ColliderGeometry& geometry, glm::vec2 localPosition, float localRotation) const
{
    ColliderStorage& colliders = *m_World->m_Colliders;

    const Id colliderId = colliders.Add();
    colliders.Get(colliderId, &ColliderColumns::Body) = {m_Handle, m_Generation};
    colliders.Get(colliderId, &ColliderColumns::Geometry) = geometry;

    Id& first = Value(&BodyColumns::FirstCollider);
    colliders.Get(colliderId, &ColliderColumns::PrevCollider) = {INVALID_HANDLE, 0};
    colliders.Get(colliderId, &ColliderColumns::NextCollider) = first;
    if (first.Handle != INVALID_HANDLE)
        colliders.Get(first, &ColliderColumns::PrevCollider) = colliderId;
    first = colliderId;
    Value(&BodyColumns::ColliderCount)++;

    colliders.Get(colliderId, &ColliderColumns::LocalPosition) = localPosition;
    colliders.Get(colliderId, &ColliderColumns::LocalRotation) = localRotation;
    // AddCollider : juste avant RecalculateMassData()
    const Collider collider(colliderId.Handle, colliderId.Generation, m_World);
    collider.RecomputeAABB();
    colliders.Get(colliderId, &ColliderColumns::Proxy) =
        m_World->m_BroadPhase->AddProxy(colliders.Get(colliderId, &ColliderColumns::AABB), colliderId.Handle);
    RecalculateMassData();
    return Collider(colliderId.Handle, colliderId.Generation, m_World);
}

void Body::DeleteCollider(const Collider& collider) const
{
    ColliderStorage& colliders = *m_World->m_Colliders;
    const Id colliderId = {collider.m_Handle, collider.m_Generation};

    assert(
        colliders.Get(colliderId, &ColliderColumns::Body).Handle == m_Handle && "Ce collider appartient à un autre body"
    );

    const Id prev = colliders.Get(colliderId, &ColliderColumns::PrevCollider);
    const Id next = colliders.Get(colliderId, &ColliderColumns::NextCollider);

    if (prev.Handle == INVALID_HANDLE)
        Value(&BodyColumns::FirstCollider) = next; // c'était le premier de la chaîne
    else
        colliders.Get(prev, &ColliderColumns::NextCollider) = next;

    if (next.Handle != INVALID_HANDLE)
        colliders.Get(next, &ColliderColumns::PrevCollider) = prev;

    Value(&BodyColumns::ColliderCount)--;
    colliders.Remove(colliderId);

    RecalculateMassData();
}

Collider Body::GetCollider(uint32_t colliderIndex) const
{
    assert(colliderIndex < Value(&BodyColumns::ColliderCount) && "Index de collider hors limites");

    const ColliderStorage& colliders = *m_World->m_Colliders;
    Id current = Value(&BodyColumns::FirstCollider);
    for (uint32_t i = 0; i < colliderIndex; ++i)
        current = colliders.Get(current, &ColliderColumns::NextCollider);

    return Collider(current.Handle, current.Generation, m_World);
}

Body::ColliderIterator& Body::ColliderIterator::operator++()
{
    const Id next = m_World->m_Colliders->Get({m_Handle, m_Generation}, &ColliderColumns::NextCollider);
    m_Handle = next.Handle;
    m_Generation = next.Generation;
    return *this;
}

Body::ColliderRange Body::GetColliders() const
{
    const Id first = Value(&BodyColumns::FirstCollider);
    return {ColliderIterator(m_World, first.Handle, first.Generation)};
}

} // namespace Crumbs2D