#include "ContactSolver.h"
#include "CrumbsMath.h"
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <algorithm>
#include <cmath>
#include <utility>
#include "Profiler.h"
namespace Crumbs2D
{
void ContactSolver::SolveContacts(
    BodyStorage& bodyStorage,
    ColliderStorage& colliderStorage,
    std::vector<Contact>& contacts,
    float deltaTime,
    float restitutionThreshold
)
{
    PROFILE_FUNCTION();
    m_Constraints.clear();

    BodyColumns& bodies = bodyStorage.Columns;

    for (Contact& contact : contacts)
    {
        const auto colliderA = std::as_const(colliderStorage).Row(contact.Colliders.First());
        const auto colliderB = std::as_const(colliderStorage).Row(contact.Colliders.Second());

        ContactConstraint& constraint = m_Constraints.emplace_back();
        constraint.Source = &contact;
        constraint.IndexA = bodyStorage.IndexOf(colliderA[&ColliderColumns::Body]);
        constraint.IndexB = bodyStorage.IndexOf(colliderB[&ColliderColumns::Body]);

        const auto bodyA = bodyStorage.RowAt(constraint.IndexA);
        const auto bodyB = bodyStorage.RowAt(constraint.IndexB);
        constraint.InvMassA = bodyA[&BodyColumns::InverseMass];
        constraint.InvMassB = bodyB[&BodyColumns::InverseMass];
        constraint.InvInertiaA = bodyA[&BodyColumns::InverseInertia];
        constraint.InvInertiaB = bodyB[&BodyColumns::InverseInertia];
        constraint.Normal = contact.Manifold.Normal;
        constraint.Tangent = {constraint.Normal.y, -constraint.Normal.x};
        constraint.FrictionCoefficient = std::sqrt(
            colliderA[&ColliderColumns::FrictionCoefficient] * colliderB[&ColliderColumns::FrictionCoefficient]
        );
        constraint.PairBounciness =
            std::max(colliderA[&ColliderColumns::Bounciness], colliderB[&ColliderColumns::Bounciness]);

        const glm::vec2 massCenterA = bodyA[&BodyColumns::MassCenter];
        const glm::vec2 massCenterB = bodyB[&BodyColumns::MassCenter];

        for (uint32_t i = 0; i < contact.Manifold.PointCount; i++)
        {
            ContactConstraintPoint& point = constraint.Points[i];
            const glm::vec2 leverArmA = contact.Manifold.Points[i].Position - massCenterA;
            const glm::vec2 leverArmB = contact.Manifold.Points[i].Position - massCenterB;
            point.NormalLeverA = glm::dot(leverArmA, constraint.Tangent);
            point.NormalLeverB = glm::dot(leverArmB, constraint.Tangent);
            point.TangentialLeverA = -glm::dot(leverArmA, constraint.Normal);
            point.TangentialLeverB = -glm::dot(leverArmB, constraint.Normal);

            point.NormalMass = 1.0f / (constraint.InvMassA + constraint.InvMassB +
                                       constraint.InvInertiaA * point.NormalLeverA * point.NormalLeverA +
                                       constraint.InvInertiaB * point.NormalLeverB * point.NormalLeverB);
            point.TangentialMass = 1.0f / (constraint.InvMassA + constraint.InvMassB +
                                           constraint.InvInertiaA * point.TangentialLeverA * point.TangentialLeverA +
                                           constraint.InvInertiaB * point.TangentialLeverB * point.TangentialLeverB);

            const float sep = contact.Manifold.Points[i].Separation;
            point.DesiredNormalVelocity =
                sep > 0.0f ? -sep / deltaTime : m_Beta / deltaTime * std::max(0.0f, -sep - m_Slop);

            point.ImpactVelocity =
                glm::dot(
                    constraint.Normal, bodies.Velocity.Data[constraint.IndexB] - bodies.Velocity.Data[constraint.IndexA]
                ) +
                bodies.AngularVelocity.Data[constraint.IndexB] * point.NormalLeverB -
                bodies.AngularVelocity.Data[constraint.IndexA] * point.NormalLeverA;

            const glm::vec2 impulse = contact.Manifold.Points[i].NormalImpulse * constraint.Normal +
                                      contact.Manifold.Points[i].TangentialImpulse * constraint.Tangent;
            const float angularImpulseA = contact.Manifold.Points[i].NormalImpulse * point.NormalLeverA +
                                          contact.Manifold.Points[i].TangentialImpulse * point.TangentialLeverA;
            const float angularImpulseB = contact.Manifold.Points[i].NormalImpulse * point.NormalLeverB +
                                          contact.Manifold.Points[i].TangentialImpulse * point.TangentialLeverB;

            bodies.Velocity.Data[constraint.IndexA] -= constraint.InvMassA * impulse;
            bodies.AngularVelocity.Data[constraint.IndexA] -= constraint.InvInertiaA * angularImpulseA;
            bodies.Velocity.Data[constraint.IndexB] += constraint.InvMassB * impulse;
            bodies.AngularVelocity.Data[constraint.IndexB] += constraint.InvInertiaB * angularImpulseB;
        }
    }

    for (int it = 0; it < m_IterationNumber; it++)
    {
        for (ContactConstraint& constraint : m_Constraints)
        {
            glm::vec2& vA = bodies.Velocity.Data[constraint.IndexA];
            float& wA = bodies.AngularVelocity.Data[constraint.IndexA];
            glm::vec2& vB = bodies.Velocity.Data[constraint.IndexB];
            float& wB = bodies.AngularVelocity.Data[constraint.IndexB];

            ContactManifold& manifold = constraint.Source->Manifold;

            for (uint32_t i = 0; i < manifold.PointCount; i++)
            {
                const ContactConstraintPoint& point = constraint.Points[i];
                // Tangential impulse
                const float impulseBound = constraint.FrictionCoefficient * manifold.Points[i].NormalImpulse;
                const float tangentialVelocity =
                    glm::dot(constraint.Tangent, vB - vA) + wB * point.TangentialLeverB - wA * point.TangentialLeverA;

                float tangentialIncrement = -point.TangentialMass * tangentialVelocity;
                const float newTangentialImpulse =
                    std::clamp(manifold.Points[i].TangentialImpulse + tangentialIncrement, -impulseBound, impulseBound);
                tangentialIncrement = newTangentialImpulse - manifold.Points[i].TangentialImpulse;
                manifold.Points[i].TangentialImpulse = newTangentialImpulse;

                vA -= constraint.InvMassA * tangentialIncrement * constraint.Tangent;
                wA -= constraint.InvInertiaA * tangentialIncrement * point.TangentialLeverA;
                vB += constraint.InvMassB * tangentialIncrement * constraint.Tangent;
                wB += constraint.InvInertiaB * tangentialIncrement * point.TangentialLeverB;

                // Normal solving
                const float normalVelocity =
                    glm::dot(constraint.Normal, vB - vA) + wB * point.NormalLeverB - wA * point.NormalLeverA;

                float normalIncrement = point.NormalMass * (point.DesiredNormalVelocity - normalVelocity);
                const float newNormalImpulse = std::max(manifold.Points[i].NormalImpulse + normalIncrement, 0.0f);
                normalIncrement = newNormalImpulse - manifold.Points[i].NormalImpulse;
                manifold.Points[i].NormalImpulse = newNormalImpulse;

                vA = vA - constraint.InvMassA * normalIncrement * constraint.Normal;
                wA = wA - constraint.InvInertiaA * normalIncrement * point.NormalLeverA;
                vB = vB + constraint.InvMassB * normalIncrement * constraint.Normal;
                wB = wB + constraint.InvInertiaB * normalIncrement * point.NormalLeverB;
            }
        }
    }

    for (const ContactConstraint& constraint : m_Constraints)
    {
        if (constraint.PairBounciness == 0.0f)
            continue;
        ContactManifold& manifold = constraint.Source->Manifold;
        glm::vec2& vA = bodies.Velocity.Data[constraint.IndexA];
        float& wA = bodies.AngularVelocity.Data[constraint.IndexA];
        glm::vec2& vB = bodies.Velocity.Data[constraint.IndexB];
        float& wB = bodies.AngularVelocity.Data[constraint.IndexB];

        for (uint32_t i = 0; i < manifold.PointCount; i++)
        {
            const ContactConstraintPoint& point = constraint.Points[i];

            if (point.ImpactVelocity > -restitutionThreshold || manifold.Points[i].NormalImpulse == 0.0f)
                continue;
            const float normalVelocity =
                glm::dot(constraint.Normal, vB - vA) + wB * point.NormalLeverB - wA * point.NormalLeverA;
            const float targetVelocity = -constraint.PairBounciness * point.ImpactVelocity;
            float increment = point.NormalMass * (targetVelocity - normalVelocity);
            const float newNormalImpulse = std::max(manifold.Points[i].NormalImpulse + increment, 0.0f);
            increment = newNormalImpulse - manifold.Points[i].NormalImpulse;
            manifold.Points[i].NormalImpulse = newNormalImpulse;

            vA -= constraint.InvMassA * increment * constraint.Normal;
            wA -= constraint.InvInertiaA * increment * point.NormalLeverA;
            vB += constraint.InvMassB * increment * constraint.Normal;
            wB += constraint.InvInertiaB * increment * point.NormalLeverB;
        }
    }
}
} // namespace Crumbs2D