#include "World.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "Integrator.h"
#include <cassert>

namespace Crumbs2D
{

World::World(const SolverSettings& solverSettings)
    : m_Bodies(CreateScope<BodyStorage>()), m_Colliders(CreateScope<ColliderStorage>()),
      m_SolverSettings(solverSettings)
{ assert(m_SolverSettings.DeltaTime > 0.0f && "DeltaTime must be positive"); }

World::~World() = default;
World::World(World&&) noexcept = default;
World& World::operator=(World&&) noexcept = default;

bool World::IsValid(const Body& body) const
{ return body.m_World == this && m_Bodies->IsValid({body.m_Handle, body.m_Generation}); }

bool World::IsValid(const Collider& collider) const
{ return collider.m_World == this && m_Colliders->IsValid({collider.m_Handle, collider.m_Generation}); }

void World::RecomputeAABBs()
{
    ColliderColumns& colliders = m_Colliders->Columns;
    const BodyStorage& bodies = *m_Bodies;

    for (uint32_t i = 0; i < m_Colliders->Count(); i++)
    {
        const Id bodyId = colliders.Body.Data[i];
        const glm::vec2 bodyPosition = bodies.Get(bodyId, &BodyColumns::Position);
        const float bodyRotation = bodies.Get(bodyId, &BodyColumns::Rotation);

        const glm::vec2 worldPosition = bodyPosition + Rotate(colliders.LocalPosition.Data[i], bodyRotation);
        const float worldRotation = bodyRotation + colliders.LocalRotation.Data[i];

        colliders.AABB.Data[i] =
            ColliderGeometry::RecomputeAABB(colliders.Geometry.Data[i], worldPosition, worldRotation);
    }
}
void World::Step()
{
    assert(m_Bodies);
    const float dt = m_SolverSettings.DeltaTime;

    Integrator::IntegrateVelocities(*m_Bodies, PhysicsSettings, dt);
    // (plus tard : broad phase, contacts, résolution des contacts)
    Integrator::IntegratePositions(*m_Bodies, dt);

    RecomputeAABBs();
}

Body World::CreateBody()
{
    const auto id = m_Bodies->Add();
    return Body(id.Handle, id.Generation, this);
}

void World::DeleteBody(const Body& body)
{
    // Détecte un body d'un autre World ; Remove vérifie la génération (double suppression).
    assert(body.m_World == this && "Body is not from this world");
    m_Bodies->Remove({body.m_Handle, body.m_Generation});
}

} // namespace Crumbs2D