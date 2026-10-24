#include "World.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "Integrator.h"
#include <cassert>
#include "BroadPhase.h"
#include "NarrowPhase.h"
#include "ContactSolver.h"
#include "Profiler.h"
namespace Crumbs2D
{

World::World(const SolverSettings& solverSettings)
    : m_Bodies(CreateScope<BodyStorage>()), m_Colliders(CreateScope<ColliderStorage>()),
      m_BroadPhase(CreateScope<BroadPhase>()), m_SolverSettings(solverSettings),
      m_ContactSolver(CreateScope<ContactSolver>(solverSettings.Iterations, solverSettings.Beta, solverSettings.Slop)),
      m_NarrowPhase(CreateScope<NarrowPhase>())
{ assert(m_SolverSettings.DeltaTime > 0.0f && "DeltaTime must be positive"); }

World::~World() = default;
World::World(World&&) noexcept = default;
World& World::operator=(World&&) noexcept = default;

bool World::IsValid(const Body& body) const
{ return body.m_World == this && m_Bodies->IsValid({body.m_Handle, body.m_Generation}); }

bool World::IsValid(const Collider& collider) const
{ return collider.m_World == this && m_Colliders->IsValid({collider.m_Handle, collider.m_Generation}); }

void World::SynchronizeColliders()
{
    ColliderColumns& colliders = m_Colliders->Columns;
    const BodyStorage& bodies = *m_Bodies;

    for (uint32_t i = 0; i < m_Colliders->Count(); i++)
    {
        const Id bodyId = colliders.Body.Data[i];
        const Transform bodyTransform = {
            bodies.Get(bodyId, &BodyColumns::Position), bodies.Get(bodyId, &BodyColumns::Rotation)
        };

        const Transform worldTransform = Transform::Compose(bodyTransform, colliders.LocalTransform.Data[i]);
        colliders.WorldTransform.Data[i] = worldTransform;
        colliders.AABB.Data[i] = ColliderGeometry::RecomputeAABB(colliders.Geometry.Data[i], worldTransform);
    }
}

void World::Step()
{
    PROFILE_FRAME();
    assert(m_Bodies);

    const float dt = m_SolverSettings.DeltaTime;

    Integrator::IntegrateVelocities(*m_Bodies, PhysicsSettings, dt);
    m_ContactSolver->SolveContacts(*m_Bodies, *m_Colliders, m_NarrowPhase->GetContacts(), dt);
    Integrator::IntegratePositions(*m_Bodies, dt);

    SynchronizeColliders();

    m_BroadPhase->UpdateProxies(*m_Colliders);

    const std::vector<ColliderPair>& pairs = m_BroadPhase->FindPairs(*m_Bodies, *m_Colliders);
    m_NarrowPhase->FindContacts(*m_Colliders, pairs);
}
Body World::CreateBody()
{
    const auto id = m_Bodies->Add();
    return Body(id.Handle, id.Generation, this);
}

void World::DeleteBody(const Body& body)
{
    assert(body.m_World == this && "Body is not from this world");
    while (body.GetColliderCount() > 0)
        body.DeleteCollider(body.GetCollider(0));
    m_Bodies->Remove({body.m_Handle, body.m_Generation});
}

std::vector<std::pair<Collider, Collider>> World::GetCandidatePairs()
{
    std::vector<std::pair<Collider, Collider>> result;
    for (const ColliderPair& pair : m_BroadPhase->GetPairs())
        result.emplace_back(
            Collider(pair.First().Handle, pair.First().Generation, this),
            Collider(pair.Second().Handle, pair.Second().Generation, this)
        );
    return result;
}

void World::ForEachBroadPhaseNode(const std::function<void(const AABB&, uint16_t, bool)>& f) const
{
    m_BroadPhase->GetTree().ForEachNode([&](const AABB& bound, uint16_t height, bool isLeaf) {
        f(bound, height, isLeaf);
    });
}

bool World::ValidateBroadPhase() const { return m_BroadPhase->GetTree().Validate(); }

std::vector<ContactInfo> World::GetContacts() const
{
    std::vector<ContactInfo> result;
    for (const Contact& contact : m_NarrowPhase->GetContacts())
    {
        ContactInfo info;
        info.First =
            Collider(contact.Colliders.First().Handle, contact.Colliders.First().Generation, const_cast<World*>(this));
        info.Second = Collider(
            contact.Colliders.Second().Handle, contact.Colliders.Second().Generation, const_cast<World*>(this)
        );
        info.Normal = contact.Manifold.Normal;
        info.PointCount = contact.Manifold.PointCount;
        for (uint32_t i = 0; i < contact.Manifold.PointCount; ++i)
        {
            info.Points[i] = contact.Manifold.Points[i].Position;
            info.Separations[i] = contact.Manifold.Points[i].Separation;
            info.FeatureIDs[i] = contact.Manifold.Points[i].FeatureID;
        }
        result.push_back(info);
    }
    return result;
}
} // namespace Crumbs2D
