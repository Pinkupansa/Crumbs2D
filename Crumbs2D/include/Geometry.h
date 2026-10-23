#pragma once
#include <cstdint>
#include <glm/vec2.hpp>
#include <cassert>
#include "CrumbsMath.h"
#include "AABB.h"
#include "Transform.h"
namespace Crumbs2D
{
enum class ShapeType : uint8_t { Circle, Box };

struct CircleData
{ float Radius = 1.0f; };

struct BoxData
{ glm::vec2 HalfExtents = {1.0f, 1.0f}; };

class ColliderGeometry
{
private:
    ShapeType m_Type;

    union
    {
        CircleData m_AsCircle;
        BoxData m_AsBox;
    };
    ColliderGeometry() : m_Type(ShapeType::Box), m_AsBox{} {}

public:
    ShapeType GetType() const { return m_Type; }
    CircleData AsCircle() const
    {
        assert(m_Type == ShapeType::Circle);
        return m_AsCircle;
    }
    BoxData AsBox() const
    {
        assert(m_Type == ShapeType::Box);
        return m_AsBox;
    }
    static ColliderGeometry MakeCircle(const CircleData& data)
    {
        ColliderGeometry geometry;
        geometry.m_Type = ShapeType::Circle;
        geometry.m_AsCircle = data;
        return geometry;
    }

    static ColliderGeometry MakeBox(const BoxData& data)
    {
        ColliderGeometry geometry;
        geometry.m_Type = ShapeType::Box;
        geometry.m_AsBox = data;
        return geometry;
    }

    // Surface de la géométrie.
    static float ComputeArea(const ColliderGeometry& geometry)
    {
        switch (geometry.m_Type)
        {
            case ShapeType::Circle: {
                const float r = geometry.m_AsCircle.Radius;
                return PI * r * r;
            }
            case ShapeType::Box: {
                const glm::vec2 h = geometry.m_AsBox.HalfExtents;
                return 4.0f * h.x * h.y;
            }
        }
        return 0.0f;
    }

    // Centre de la géométrie dans son propre repère.
    static glm::vec2 ComputeCenter(const ColliderGeometry& geometry)
    {
        switch (geometry.m_Type)
        {
            case ShapeType::Circle:
            case ShapeType::Box:
                return {0.0f, 0.0f};
        }
        return {0.0f, 0.0f};
    }

    // Moment d'inertie par unité de densité, autour du centre de la géométrie.
    static float ComputeUnitInertia(const ColliderGeometry& geometry)
    {
        switch (geometry.m_Type)
        {
            case ShapeType::Circle: {
                const float r = geometry.m_AsCircle.Radius;
                return 0.5f * PI * r * r * r * r;
            }
            case ShapeType::Box: {
                const glm::vec2 h = geometry.m_AsBox.HalfExtents;
                return ComputeArea(geometry) * (h.x * h.x + h.y * h.y) / 3.0f;
            }
        }
        return 0.0f;
    }

    static AABB RecomputeAABB(const ColliderGeometry& geometry, const Transform& worldTransform)
    {
        float globalRotation = worldTransform.Rotation;
        glm::vec2 globalPosition = worldTransform.Position;
        switch (geometry.GetType())
        {
            case ShapeType::Box: {
                glm::vec2 h = geometry.AsBox().HalfExtents;
                float absin = fabs(sin(globalRotation));
                float abcos = fabs(cos(globalRotation));
                glm::vec2 extent = {abcos * h.x + absin * h.y, absin * h.x + abcos * h.y};
                return AABB(globalPosition - extent, globalPosition + extent);
            }

            case ShapeType::Circle: {
                glm::vec2 extent = glm::vec2(geometry.AsCircle().Radius);
                return AABB(globalPosition - extent, globalPosition + extent);
            }
            default:
                assert(false);
                return AABB({0, 0}, {0, 0});
        }
    }
};
} // namespace Crumbs2D