#pragma once
#include <glm/vec2.hpp>
#include <cstdint>
#include <array>
#include "HandleTable.h"

namespace Crumbs2D
{
struct ContactPoint
{
    glm::vec2 Position = glm::vec2(0.0f);
    float NormalImpulse, TangentialImpulse = 0;
    float Separation = 0;
    uint32_t FeatureID = 0;
};

struct ContactManifold
{
    glm::vec2 Normal = glm::vec2(0.0f);
    uint32_t PointCount = 0;
    std::array<ContactPoint, 2> Points;
};

class ColliderPair
{
private:
    Id m_ColliderA, m_ColliderB;

public:
    ColliderPair(Id A, Id B)
    {
        if (A.Handle < B.Handle)
        {
            m_ColliderA = A;
            m_ColliderB = B;
        }
        else
        {
            m_ColliderA = B;
            m_ColliderB = A;
        }
    }

    const Id First() const { return m_ColliderA; }
    const Id Second() const { return m_ColliderB; };
    const uint64_t MakePairKey() const
    { return (static_cast<uint64_t>(m_ColliderA.Handle) << 32) | m_ColliderB.Handle; }
};

struct Contact
{
    ColliderPair Colliders;
    ContactManifold Manifold;
};

} // namespace Crumbs2D