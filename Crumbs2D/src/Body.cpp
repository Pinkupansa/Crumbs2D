#include "Body.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "CrumbsMath.h"
#include "World.h"
#include "WorldWrites.h"
#include <cassert>
#include <utility>

namespace Crumbs2D
{
template <typename T> const T& Body::Value(Column<T> BodyColumns::* column) const
{
    assert(m_World != nullptr && "Body is not attached to a world");
    return m_World->m_Bodies->Get({m_Handle, m_Generation}, column);
}

bool Body::IsValid() const { return m_World != nullptr && m_World->IsValid(*this); }

void Body::SetPosition(glm::vec2 position) const
{ m_World->SetBodyValue<&BodyColumns::Position>({m_Handle, m_Generation}, position); }
glm::vec2 Body::GetPosition() const { return Value(&BodyColumns::Position); }

void Body::SetRotation(float rotation) const
{ m_World->SetBodyValue<&BodyColumns::Rotation>({m_Handle, m_Generation}, rotation); }
float Body::GetRotation() const { return Value(&BodyColumns::Rotation); }

Transform Body::GetTransform() const
{
    const auto row = std::as_const(*m_World->m_Bodies).Row({m_Handle, m_Generation});
    return {row[&BodyColumns::Position], row[&BodyColumns::Rotation]};
}

void Body::SetVelocity(glm::vec2 velocity) const
{ m_World->SetBodyValue<&BodyColumns::Velocity>({m_Handle, m_Generation}, velocity); }
glm::vec2 Body::GetVelocity() const { return Value(&BodyColumns::Velocity); }

void Body::SetAngularVelocity(float angularVelocity) const
{ m_World->SetBodyValue<&BodyColumns::AngularVelocity>({m_Handle, m_Generation}, angularVelocity); }
float Body::GetAngularVelocity() const { return Value(&BodyColumns::AngularVelocity); }

void Body::SetMass(float mass) const { m_World->SetBodyValue<&BodyColumns::Mass>({m_Handle, m_Generation}, mass); }
float Body::GetMass() const { return Value(&BodyColumns::Mass); }

void Body::SetIsStatic(bool isStatic) const
{ m_World->SetBodyValue<&BodyColumns::IsStatic>({m_Handle, m_Generation}, static_cast<uint8_t>(isStatic ? 1 : 0)); }
bool Body::IsStatic() const { return Value(&BodyColumns::IsStatic) != 0; }

void Body::AddForce(glm::vec2 force) const
{ m_World->SetBodyValue<&BodyColumns::Force>({m_Handle, m_Generation}, Value(&BodyColumns::Force) + force); }

void Body::AddTorque(float torque) const
{ m_World->SetBodyValue<&BodyColumns::Torque>({m_Handle, m_Generation}, Value(&BodyColumns::Torque) + torque); }

void Body::AddImpulse(glm::vec2 impulse) const
{
    m_World->EnsureSynchronized();
    m_World->SetBodyValue<&BodyColumns::Velocity>(
        {m_Handle, m_Generation}, Value(&BodyColumns::Velocity) + impulse * Value(&BodyColumns::InverseMass)
    );
}

void Body::AddAngularImpulse(float impulse) const
{
    m_World->EnsureSynchronized();
    m_World->SetBodyValue<&BodyColumns::AngularVelocity>(
        {m_Handle, m_Generation}, Value(&BodyColumns::AngularVelocity) + impulse * Value(&BodyColumns::InverseInertia)
    );
}

void Body::AddForceAtPoint(glm::vec2 force, glm::vec2 point) const
{
    m_World->EnsureSynchronized();
    AddTorque(Cross(point - Value(&BodyColumns::MassCenter), force));
    AddForce(force);
}

void Body::AddImpulseAtPoint(glm::vec2 impulse, glm::vec2 point) const
{
    m_World->EnsureSynchronized();
    AddAngularImpulse(Cross(point - Value(&BodyColumns::MassCenter), impulse));
    AddImpulse(impulse);
}

Collider Body::AddCollider(const ColliderGeometry& geometry, const Transform& localTransform) const
{ return m_World->AllocateCollider({m_Handle, m_Generation}, geometry, localTransform); }

void Body::DeleteCollider(const Collider& collider) const
{
    const Id colliderId = {collider.m_Handle, collider.m_Generation};
    assert(
        m_World->m_Colliders->Get(colliderId, &ColliderColumns::Body).Handle == m_Handle &&
        "Collider belongs to another body"
    );
    m_World->RequestDestroyCollider(colliderId);
}

uint32_t Body::GetColliderCount() const { return Value(&BodyColumns::ColliderCount); }

Collider Body::GetCollider(uint32_t colliderIndex) const
{
    assert(colliderIndex < Value(&BodyColumns::ColliderCount) && "Collider index out of range");

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