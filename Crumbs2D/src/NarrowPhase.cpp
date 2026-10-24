#include "NarrowPhase.h"
#include "Geometry.h"
#include "Constants.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <limits>
#include "Profiler.h"
namespace Crumbs2D
{
static ContactManifold FindCircleCircle(
    CircleData firstCircle,
    const Transform& worldTransformFirst,
    CircleData secondCircle,
    const Transform& worldTransformSecond
)
{
    ContactManifold manifold;

    glm::vec2 secondCircleLocal = Transform::ApplyInverseToPoint(worldTransformFirst, worldTransformSecond.Position);
    float distanceSquared = glm::dot(secondCircleLocal, secondCircleLocal);
    float maxDist = firstCircle.Radius + secondCircle.Radius + SPECULATIVE_DISTANCE;
    if (distanceSquared >= maxDist * maxDist)
        return manifold;

    float distance = std::sqrt(distanceSquared);
    float separation = distance - (firstCircle.Radius + secondCircle.Radius);
    glm::vec2 normalLocal = distance > EPSILON ? secondCircleLocal / distance : glm::vec2{1.0f, 0.0f};
    glm::vec2 contactPointLocal = (firstCircle.Radius + separation / 2.0f) * normalLocal;
    manifold.PointCount = 1;
    manifold.Normal = Transform::ApplyToVector(worldTransformFirst, normalLocal);
    manifold.Points[0].Position = Transform::ApplyToPoint(worldTransformFirst, contactPointLocal);
    manifold.Points[0].Separation = separation;
    manifold.Points[0].FeatureID = 0;

    return manifold;
}
static ContactManifold
FindBoxCircle(BoxData box, const Transform& worldTransformBox, CircleData circle, const Transform& worldTransformCircle)
{
    ContactManifold manifold;
    glm::vec2 circleLocal = Transform::ApplyInverseToPoint(worldTransformBox, worldTransformCircle.Position);
    glm::vec2 projectionLocal = glm::clamp(circleLocal, -box.HalfExtents, box.HalfExtents);
    glm::vec2 deltaLocal = circleLocal - projectionLocal;
    float distanceSquared = glm::dot(deltaLocal, deltaLocal);

    glm::vec2 normalLocal;
    float separation;

    if (distanceSquared > EPSILON * EPSILON)
    {
        float maxDistance = circle.Radius + SPECULATIVE_DISTANCE;
        if (distanceSquared > maxDistance * maxDistance)
            return manifold;

        float distance = std::sqrt(distanceSquared);
        normalLocal = deltaLocal / distance;
        separation = distance - circle.Radius;
    }
    else
    {
        glm::vec2 faceDistances = box.HalfExtents - glm::abs(circleLocal);
        int axis = faceDistances.x < faceDistances.y ? 0 : 1;
        float side = circleLocal[axis] >= 0.0f ? 1.0f : -1.0f;

        normalLocal = {0.0f, 0.0f};
        normalLocal[axis] = side;
        projectionLocal[axis] = side * box.HalfExtents[axis];
        separation = -(faceDistances[axis] + circle.Radius);
    }

    glm::vec2 contactPointLocal = projectionLocal + separation / 2.0f * normalLocal;

    manifold.Normal = Transform::ApplyToVector(worldTransformBox, normalLocal);
    manifold.PointCount = 1;
    manifold.Points[0].Position = Transform::ApplyToPoint(worldTransformBox, contactPointLocal);
    manifold.Points[0].Separation = separation;
    manifold.Points[0].FeatureID = 0;
    return manifold;
}

static ContactManifold FindBoxBox(
    BoxData firstBox, const Transform& worldTransformFirst, BoxData secondBox, const Transform& worldTransformSecond
)
{
    ContactManifold manifold;

    const Transform relativeTransformSecond = Transform::Relative(worldTransformFirst, worldTransformSecond);
    const glm::vec2 secondCenter = relativeTransformSecond.Position;
    const glm::vec2 secondRight = relativeTransformSecond.Right();
    const glm::vec2 secondUp = relativeTransformSecond.Up();
    const glm::vec2 firstHalf = firstBox.HalfExtents;
    const glm::vec2 secondHalf = secondBox.HalfExtents;

    const glm::vec2 firstVertices[4] = {
        {-firstHalf.x, -firstHalf.y},
        {firstHalf.x, -firstHalf.y},
        {firstHalf.x, firstHalf.y},
        {-firstHalf.x, firstHalf.y}
    };
    const glm::vec2 firstNormals[4] = {{0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}};

    const glm::vec2 secondVertices[4] = {
        secondCenter - secondHalf.x * secondRight - secondHalf.y * secondUp,
        secondCenter + secondHalf.x * secondRight - secondHalf.y * secondUp,
        secondCenter + secondHalf.x * secondRight + secondHalf.y * secondUp,
        secondCenter - secondHalf.x * secondRight + secondHalf.y * secondUp
    };
    const glm::vec2 secondNormals[4] = {-secondUp, secondRight, secondUp, -secondRight};

    auto findMaxSeparation =
        [](const glm::vec2* vertices, const glm::vec2* normals, const glm::vec2* otherVertices, int& bestFace) {
            float maxSeparation = -std::numeric_limits<float>::max();
            for (int face = 0; face < 4; face++)
            {
                float faceSeparation = std::numeric_limits<float>::max();
                for (int vertex = 0; vertex < 4; vertex++)
                    faceSeparation =
                        std::min(faceSeparation, glm::dot(normals[face], otherVertices[vertex] - vertices[face]));

                if (faceSeparation > maxSeparation)
                {
                    maxSeparation = faceSeparation;
                    bestFace = face;
                }
            }
            return maxSeparation;
        };

    int firstFace = 0;
    const float firstSeparation = findMaxSeparation(firstVertices, firstNormals, secondVertices, firstFace);
    if (firstSeparation > SPECULATIVE_DISTANCE)
        return manifold;

    int secondFace = 0;
    const float secondSeparation = findMaxSeparation(secondVertices, secondNormals, firstVertices, secondFace);
    if (secondSeparation > SPECULATIVE_DISTANCE)
        return manifold;

    const bool referenceIsSecond = secondSeparation > firstSeparation;
    const glm::vec2* referenceVertices = referenceIsSecond ? secondVertices : firstVertices;
    const glm::vec2* referenceNormals = referenceIsSecond ? secondNormals : firstNormals;
    const glm::vec2* incidentVertices = referenceIsSecond ? firstVertices : secondVertices;
    const glm::vec2* incidentNormals = referenceIsSecond ? firstNormals : secondNormals;
    const int referenceFace = referenceIsSecond ? secondFace : firstFace;

    const glm::vec2 referenceNormal = referenceNormals[referenceFace];
    const glm::vec2 referenceStart = referenceVertices[referenceFace];
    const glm::vec2 referenceEnd = referenceVertices[(referenceFace + 1) % 4];

    // the incident face is the face the normal of which is the least alighned with the reference face
    int incidentFace = 0;
    float minAlignment = std::numeric_limits<float>::max();
    for (int face = 0; face < 4; face++)
    {
        const float alignment = glm::dot(incidentNormals[face], referenceNormal);
        if (alignment < minAlignment)
        {
            minAlignment = alignment;
            incidentFace = face;
        }
    }

    const glm::vec2 incidentStart = incidentVertices[incidentFace];
    const glm::vec2 incidentEnd = incidentVertices[(incidentFace + 1) % 4];
    const float referenceLength = glm::length(referenceEnd - referenceStart);
    const glm::vec2 referenceTangent = (referenceEnd - referenceStart) / referenceLength;

    const float startCoordinate = glm::dot(referenceTangent, incidentStart - referenceStart);
    const float endCoordinate = glm::dot(referenceTangent, incidentEnd - referenceStart);

    // fraction of the incident segment to keep as contact point
    float enter = 0.0f;
    float exit = 1.0f;
    if (std::abs(endCoordinate - startCoordinate) > EPSILON)
    {
        const float atStart = -startCoordinate / (endCoordinate - startCoordinate);
        const float atEnd = (referenceLength - startCoordinate) / (endCoordinate - startCoordinate);
        enter = std::max(enter, std::min(atStart, atEnd));
        exit = std::min(exit, std::max(atStart, atEnd));
    }
    if (enter > exit)
        return manifold;

    const glm::vec2 normalLocal = referenceIsSecond ? -referenceNormal : referenceNormal;
    manifold.Normal = Transform::ApplyToVector(worldTransformFirst, normalLocal);

    const float parameters[2] = {enter, exit};
    for (int i = 0; i < 2; i++)
    {
        const glm::vec2 incidentPoint = glm::mix(incidentStart, incidentEnd, parameters[i]);
        const float pointSeparation = glm::dot(referenceNormal, incidentPoint - referenceStart);
        if (pointSeparation > SPECULATIVE_DISTANCE) // reject : second point not in contact, only one contact point
            continue;

        ContactPoint& point = manifold.Points[manifold.PointCount++];
        point.Position =
            Transform::ApplyToPoint(worldTransformFirst, incidentPoint - pointSeparation / 2.0f * referenceNormal);
        point.Separation = pointSeparation;
        point.FeatureID = static_cast<uint32_t>(referenceFace) | static_cast<uint32_t>(incidentFace) << 2 |
                          static_cast<uint32_t>(i) << 4 | static_cast<uint32_t>(referenceIsSecond) << 6;
    }

    return manifold;
}
static ContactManifold FindContact(
    const ColliderGeometry& geometryFirst,
    const Transform& worldTransformFirst,
    const ColliderGeometry& geometrySecond,
    const Transform& worldTransformSecond
)
{
    switch (geometryFirst.GetType())
    {
        case ShapeType::Box:
            switch (geometrySecond.GetType())
            {
                case ShapeType::Box:
                    return FindBoxBox(
                        geometryFirst.AsBox(), worldTransformFirst, geometrySecond.AsBox(), worldTransformSecond
                    );

                case ShapeType::Circle:
                    return FindBoxCircle(
                        geometryFirst.AsBox(), worldTransformFirst, geometrySecond.AsCircle(), worldTransformSecond
                    );
            }
            break;
        case ShapeType::Circle:
            switch (geometrySecond.GetType())
            {
                case ShapeType::Box: {
                    ContactManifold manifold = FindBoxCircle(
                        geometrySecond.AsBox(), worldTransformSecond, geometryFirst.AsCircle(), worldTransformFirst
                    );
                    manifold.Normal = -manifold.Normal; // normal must go from smaller to bigger to fit solver
                    return manifold;
                }

                case ShapeType::Circle:
                    return FindCircleCircle(
                        geometryFirst.AsCircle(), worldTransformFirst, geometrySecond.AsCircle(), worldTransformSecond
                    );
            }
            break;
    }
    return {};
}
const std::vector<Contact>&
NarrowPhase::FindContacts(const ColliderStorage& colliderStorage, const std::vector<ColliderPair>& candidates)
{
    PROFILE_FUNCTION();
    m_CurrentSwapchainIndex = 1 - m_CurrentSwapchainIndex;

    std::vector<Contact>& currentBuffer = m_ContactBufferSwapchain[m_CurrentSwapchainIndex];
    std::unordered_map<uint64_t, uint32_t>& currentCache = m_ContactCacheSwapchain[m_CurrentSwapchainIndex];
    currentBuffer.clear();
    currentCache.clear();

    const std::vector<Contact>& oldBuffer = m_ContactBufferSwapchain[1 - m_CurrentSwapchainIndex];
    const std::unordered_map<uint64_t, uint32_t>& oldCache = m_ContactCacheSwapchain[1 - m_CurrentSwapchainIndex];

    const auto& worldTransform = [&](Id collider) {
        return colliderStorage.Get(collider, &ColliderColumns::WorldTransform);
    };

    for (const ColliderPair& pair : candidates)
    {
        ContactManifold manifold = FindContact(
            colliderStorage.Get(pair.First(), &ColliderColumns::Geometry),
            worldTransform(pair.First()),
            colliderStorage.Get(pair.Second(), &ColliderColumns::Geometry),
            worldTransform(pair.Second())
        );

        if (manifold.PointCount > 0)
        {
            uint64_t key = pair.MakePairKey();
            const auto oldIterator = oldCache.find(key);
            if (oldIterator != oldCache.end())
            {
                const ContactManifold& oldManifold = oldBuffer[oldIterator->second].Manifold;
                for (uint32_t i = 0; i < manifold.PointCount; i++)
                    for (uint32_t j = 0; j < oldManifold.PointCount; j++)
                        if (oldManifold.Points[j].FeatureID == manifold.Points[i].FeatureID)
                        {
                            manifold.Points[i].NormalImpulse = oldManifold.Points[j].NormalImpulse;
                            manifold.Points[i].TangentialImpulse = oldManifold.Points[j].TangentialImpulse;
                            break;
                        }
            }
            currentCache.insert({key, currentBuffer.size()});
            currentBuffer.push_back({pair, manifold});
        }
    }
    return m_ContactBufferSwapchain[m_CurrentSwapchainIndex];
}
} // namespace Crumbs2D