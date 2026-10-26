#include "World.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "Integrator.h"
#include "BroadPhase.h"
#include "NarrowPhase.h"
#include "ContactSolver.h"
#include "CrumbsMath.h"
#include "Profiler.h"

#include <glm/geometric.hpp>

#include <cassert>
#include <utility>

namespace Crumbs2D
{

World::World(const SolverSettings& solverSettings)
    : m_Bodies(CreateScope<BodyStorage>()), m_Colliders(CreateScope<ColliderStorage>()),
      m_BroadPhase(CreateScope<BroadPhase>()), m_NarrowPhase(CreateScope<NarrowPhase>()),
      m_ContactSolver(CreateScope<ContactSolver>(solverSettings.Iterations, solverSettings.Beta, solverSettings.Slop)),
      m_SolverSettings(solverSettings)
{ assert(m_SolverSettings.DeltaTime > 0.0f && "DeltaTime must be positive"); }

World::~World() = default;
World::World(World&&) noexcept = default;
World& World::operator=(World&&) noexcept = default;

bool World::IsValid(const Body& body) const
{
    const Id id = {body.m_Handle, body.m_Generation};
    return body.m_World == this && m_Bodies->IsValid(id) &&
           m_Bodies->Get(id, &BodyColumns::State) != LifeState::PendingDestroy;
}

bool World::IsValid(const Collider& collider) const
{
    const Id id = {collider.m_Handle, collider.m_Generation};
    return collider.m_World == this && m_Colliders->IsValid(id) &&
           m_Colliders->Get(id, &ColliderColumns::State) != LifeState::PendingDestroy;
}

void World::MarkBodyDirty(Id body, uint8_t flags)
{
    uint8_t& dirty = m_Bodies->Get(body, &BodyColumns::DirtyFlags);
    if (dirty == 0)
        m_DirtyBodies.push_back(body);
    dirty |= flags;
}

void World::MarkColliderDirty(Id collider, uint8_t flags)
{
    uint8_t& dirty = m_Colliders->Get(collider, &ColliderColumns::DirtyFlags);
    if (dirty == 0)
        m_DirtyColliders.push_back(collider);
    dirty |= flags;
}

Collider World::AllocateCollider(Id body, const ColliderGeometry& geometry, const Transform& localTransform)
{
    const Id id = m_Colliders->Add();
    const auto collider = m_Colliders->Row(id);
    const auto owner = m_Bodies->Row(body);

    collider[&ColliderColumns::Body] = body;
    collider[&ColliderColumns::Geometry] = geometry;
    collider[&ColliderColumns::LocalTransform] = localTransform;

    Id& first = owner[&BodyColumns::FirstCollider];
    collider[&ColliderColumns::PrevCollider] = {INVALID_HANDLE, 0};
    collider[&ColliderColumns::NextCollider] = first;
    if (first.Handle != INVALID_HANDLE)
        m_Colliders->Get(first, &ColliderColumns::PrevCollider) = id;
    first = id;
    owner[&BodyColumns::ColliderCount]++;

    m_PendingColliders.push_back(id);
    MarkColliderDirty(id, ColliderDirty::Placement);
    MarkBodyDirty(body, BodyDirty::Mass);

    return Collider(id.Handle, id.Generation, this);
}

void World::RequestDestroyCollider(Id collider)
{
    const auto row = m_Colliders->Row(collider);
    if (row[&ColliderColumns::State] == LifeState::PendingDestroy)
        return;

    const Id body = row[&ColliderColumns::Body];
    const Id prev = row[&ColliderColumns::PrevCollider];
    const Id next = row[&ColliderColumns::NextCollider];
    const auto owner = m_Bodies->Row(body);
    if (prev.Handle == INVALID_HANDLE)
        owner[&BodyColumns::FirstCollider] = next;
    else
        m_Colliders->Get(prev, &ColliderColumns::NextCollider) = next;
    if (next.Handle != INVALID_HANDLE)
        m_Colliders->Get(next, &ColliderColumns::PrevCollider) = prev;
    row[&ColliderColumns::PrevCollider] = {INVALID_HANDLE, 0};
    row[&ColliderColumns::NextCollider] = {INVALID_HANDLE, 0};
    owner[&BodyColumns::ColliderCount]--;

    row[&ColliderColumns::State] = LifeState::PendingDestroy;
    m_CollidersToDestroy.push_back(collider);
    MarkBodyDirty(body, BodyDirty::Mass);
}

void World::RequestDestroyBody(Id body)
{
    if (m_Bodies->Get(body, &BodyColumns::State) == LifeState::PendingDestroy)
        return;

    Id current = m_Bodies->Get(body, &BodyColumns::FirstCollider);
    while (current.Handle != INVALID_HANDLE)
    {
        const Id next = m_Colliders->Get(current, &ColliderColumns::NextCollider);
        RequestDestroyCollider(current);
        current = next;
    }

    m_Bodies->Get(body, &BodyColumns::State) = LifeState::PendingDestroy;
    m_BodiesToDestroy.push_back(body);
}

template <typename F> void World::ForEachColliderOf(Id body, F&& f) const
{
    for (Id collider = m_Bodies->Get(body, &BodyColumns::FirstCollider); collider.Handle != INVALID_HANDLE;
         collider = m_Colliders->Get(collider, &ColliderColumns::NextCollider))
        f(collider);
}

void World::EnsureSynchronized()
{
    if (!m_DirtyBodies.empty() || !m_DirtyColliders.empty())
        SynchronizeDirty();
}

void World::RecalculateMassData(Id body)
{
    const auto row = m_Bodies->Row(body);
    const bool isStatic = row[&BodyColumns::IsStatic] != 0;
    const float mass = row[&BodyColumns::Mass];

    float totalArea = 0.0f;
    glm::vec2 weightedCenter = {0.0f, 0.0f};
    ForEachColliderOf(body, [&](Id id) {
        const auto collider = std::as_const(*m_Colliders).Row(id);
        const ColliderGeometry& geometry = collider[&ColliderColumns::Geometry];
        const float area = ColliderGeometry::ComputeArea(geometry);
        totalArea += area;
        weightedCenter +=
            area * Transform::ApplyToPoint(
                       collider[&ColliderColumns::LocalTransform], ColliderGeometry::ComputeCenter(geometry)
                   );
    });

    row[&BodyColumns::InverseMass] = (!isStatic && mass > 0.0f) ? 1.0f / mass : 0.0f;

    if (totalArea <= 0.0f)
    {
        row[&BodyColumns::MassCenterLocal] = glm::vec2(0.0f);
        row[&BodyColumns::MassCenter] = row[&BodyColumns::Position];
        row[&BodyColumns::InverseInertia] = 0.0f;
        return;
    }

    const glm::vec2 massCenterLocal = weightedCenter / totalArea;
    const float density = mass / totalArea;
    float totalInertia = 0.0f;
    ForEachColliderOf(body, [&](Id id) {
        const auto collider = std::as_const(*m_Colliders).Row(id);
        const ColliderGeometry& geometry = collider[&ColliderColumns::Geometry];
        const float colliderMass = density * ColliderGeometry::ComputeArea(geometry);
        const glm::vec2 offset =
            Transform::ApplyToPoint(
                collider[&ColliderColumns::LocalTransform], ColliderGeometry::ComputeCenter(geometry)
            ) -
            massCenterLocal;
        totalInertia +=
            density * ColliderGeometry::ComputeUnitInertia(geometry) + colliderMass * glm::dot(offset, offset);
    });

    row[&BodyColumns::MassCenterLocal] = massCenterLocal;
    row[&BodyColumns::MassCenter] = row[&BodyColumns::Position] + Rotate(massCenterLocal, row[&BodyColumns::Rotation]);
    row[&BodyColumns::InverseInertia] = (!isStatic && totalInertia > 0.0f) ? 1.0f / totalInertia : 0.0f;
}

void World::RefreshColliderPlacement(RowView<ColliderColumns> collider)
{
    const auto body = std::as_const(*m_Bodies).Row(collider[&ColliderColumns::Body]);
    const Transform bodyTransform = {body[&BodyColumns::Position], body[&BodyColumns::Rotation]};
    const Transform worldTransform = Transform::Compose(bodyTransform, collider[&ColliderColumns::LocalTransform]);
    collider[&ColliderColumns::WorldTransform] = worldTransform;
    collider[&ColliderColumns::AABB] =
        ColliderGeometry::RecomputeAABB(collider[&ColliderColumns::Geometry], worldTransform);
}

void World::SynchronizeColliders()
{
    PROFILE_FUNCTION();
    for (uint32_t i = 0; i < m_Colliders->Count(); i++)
        RefreshColliderPlacement(m_Colliders->RowAt(i));
}

void World::ApplyPendingChanges()
{
    PROFILE_FUNCTION();
    if (!m_CollidersToDestroy.empty())
    {
        m_NarrowPhase->RemoveContactsIf([&](const Contact& contact) {
            return m_Colliders->Get(contact.Colliders.First(), &ColliderColumns::State) == LifeState::PendingDestroy ||
                   m_Colliders->Get(contact.Colliders.Second(), &ColliderColumns::State) == LifeState::PendingDestroy;
        });
    }

    for (const Id& collider : m_CollidersToDestroy)
    {
        if (!m_Colliders->IsValid(collider))
            continue;
        const uint32_t proxy = m_Colliders->Get(collider, &ColliderColumns::Proxy);
        if (proxy != UINT32_MAX)
            m_BroadPhase->RemoveProxy(proxy);
        m_Colliders->Remove(collider);
    }

    for (const Id& body : m_BodiesToDestroy)
    {
        if (m_Bodies->IsValid(body))
            m_Bodies->Remove(body);
    }

    for (const Id& body : m_PendingBodies)
    {
        if (m_Bodies->IsValid(body))
            m_Bodies->Get(body, &BodyColumns::State) = LifeState::Active;
    }

    for (const Id& collider : m_PendingColliders)
    {
        if (!m_Colliders->IsValid(collider))
            continue;
        m_Colliders->Get(collider, &ColliderColumns::State) = LifeState::Active;
        MarkColliderDirty(collider, ColliderDirty::Placement);
    }

    m_CollidersToDestroy.clear();
    m_BodiesToDestroy.clear();
    m_PendingBodies.clear();
    m_PendingColliders.clear();
}

void World::SynchronizeDirty()
{
    PROFILE_FUNCTION();
    for (const Id& body : m_DirtyBodies)
    {
        if (!m_Bodies->IsValid(body))
            continue;

        const auto row = m_Bodies->Row(body);
        const uint8_t flags = row[&BodyColumns::DirtyFlags];
        row[&BodyColumns::DirtyFlags] = 0;

        if (flags & BodyDirty::Mass)
            RecalculateMassData(body);
        if (flags & BodyDirty::Transform)
        {
            row[&BodyColumns::MassCenter] =
                row[&BodyColumns::Position] + Rotate(row[&BodyColumns::MassCenterLocal], row[&BodyColumns::Rotation]);
            ForEachColliderOf(body, [&](Id collider) { MarkColliderDirty(collider, ColliderDirty::Placement); });
        }
    }
    m_DirtyBodies.clear();

    for (const Id& id : m_DirtyColliders)
    {
        if (!m_Colliders->IsValid(id))
            continue;

        const auto collider = m_Colliders->Row(id);
        const uint8_t flags = collider[&ColliderColumns::DirtyFlags];
        collider[&ColliderColumns::DirtyFlags] = 0;

        if (!(flags & ColliderDirty::Placement))
            continue;

        RefreshColliderPlacement(collider);

        if (collider[&ColliderColumns::State] != LifeState::Active)
            continue;
        uint32_t& proxy = collider[&ColliderColumns::Proxy];
        if (proxy == UINT32_MAX)
            proxy = m_BroadPhase->AddProxy(collider[&ColliderColumns::AABB], id.Handle);
        else
            m_BroadPhase->MoveProxy(proxy, collider[&ColliderColumns::AABB]);
    }
    m_DirtyColliders.clear();
}

// TODO: exercise 3 (see the refactoring plan).
bool World::ValidateInvariants() const { return true; }

void World::Step()
{
    PROFILE_FRAME();
    const float dt = m_SolverSettings.DeltaTime;

    ApplyPendingChanges();
    SynchronizeDirty();

    Integrator::IntegrateVelocities(*m_Bodies, PhysicsSettings, dt);
    m_ContactSolver->SolveContacts(
        *m_Bodies, *m_Colliders, m_NarrowPhase->GetContacts(), dt, PhysicsSettings.RestitutionThreshold
    );
    Integrator::IntegratePositions(*m_Bodies, dt);

    SynchronizeColliders();
    m_BroadPhase->UpdateProxies(*m_Colliders);

    const std::vector<ColliderPair>& pairs = m_BroadPhase->FindPairs(*m_Bodies, *m_Colliders);
    m_NarrowPhase->FindContacts(*m_Colliders, pairs);
}

Body World::CreateBody()
{
    const Id id = m_Bodies->Add();
    m_PendingBodies.push_back(id);
    MarkBodyDirty(id, BodyDirty::Mass);
    return Body(id.Handle, id.Generation, this);
}

void World::DeleteBody(const Body& body)
{
    assert(body.m_World == this && "Body is not from this world");
    RequestDestroyBody({body.m_Handle, body.m_Generation});
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