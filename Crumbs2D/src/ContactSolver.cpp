#include "ContactSolver.h"
#include "CrumbsMath.h"
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <algorithm>
namespace Crumbs2D
{
// ContactSolver.cpp
void ContactSolver::SolveContacts(
    BodyStorage& bodyStorage, ColliderStorage& colliderStorage, std::vector<Contact>& contacts, float deltaTime
)
{
    m_Constraints.clear();

    for (Contact& contact : contacts)
    {
        ContactConstraint& constraint = m_Constraints.emplace_back();
        constraint.Source = &contact;
        constraint.BodyA = colliderStorage.Get(contact.Colliders.First(), &ColliderColumns::Body);
        constraint.BodyB = colliderStorage.Get(contact.Colliders.Second(), &ColliderColumns::Body);
        constraint.InvMassA = bodyStorage.Get(constraint.BodyA, &BodyColumns::InverseMass);
        constraint.InvMassB = bodyStorage.Get(constraint.BodyB, &BodyColumns::InverseMass);
        constraint.InvInertiaA = bodyStorage.Get(constraint.BodyA, &BodyColumns::InverseInertia);
        constraint.InvInertiaB = bodyStorage.Get(constraint.BodyB, &BodyColumns::InverseInertia);
        constraint.Normal = contact.Manifold.Normal;
        constraint.Tangent = Rotate(constraint.Normal, -PI / 2.0f);
        constraint.FrictionCoefficient = std::sqrt(
            colliderStorage.Get(contact.Colliders.First(), &ColliderColumns::FrictionCoefficient) *
            colliderStorage.Get(contact.Colliders.Second(), &ColliderColumns::FrictionCoefficient)
        );

        const glm::vec2 massCenterA = bodyStorage.Get(constraint.BodyA, &BodyColumns::MassCenter);
        const glm::vec2 massCenterB = bodyStorage.Get(constraint.BodyB, &BodyColumns::MassCenter);

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
            const glm::vec2 impulse = contact.Manifold.Points[i].NormalImpulse * constraint.Normal +
                                      contact.Manifold.Points[i].TangentialImpulse * constraint.Tangent;
            const float angularImpulseA = contact.Manifold.Points[i].NormalImpulse * point.NormalLeverA +
                                          contact.Manifold.Points[i].TangentialImpulse * point.TangentialLeverA;
            const float angularImpulseB = contact.Manifold.Points[i].NormalImpulse * point.NormalLeverB +
                                          contact.Manifold.Points[i].TangentialImpulse * point.TangentialLeverB;

            bodyStorage.Get(constraint.BodyA, &BodyColumns::Velocity) -= constraint.InvMassA * impulse;
            bodyStorage.Get(constraint.BodyA, &BodyColumns::AngularVelocity) -=
                constraint.InvInertiaA * angularImpulseA;
            bodyStorage.Get(constraint.BodyB, &BodyColumns::Velocity) += constraint.InvMassB * impulse;
            bodyStorage.Get(constraint.BodyB, &BodyColumns::AngularVelocity) +=
                constraint.InvInertiaB * angularImpulseB;
        }
    }

    for (int it = 0; it < m_IterationNumber; it++)
    {
        for (ContactConstraint& constraint : m_Constraints)
        {
            glm::vec2& vA = bodyStorage.Get(constraint.BodyA, &BodyColumns::Velocity);
            float& wA = bodyStorage.Get(constraint.BodyA, &BodyColumns::AngularVelocity);
            glm::vec2& vB = bodyStorage.Get(constraint.BodyB, &BodyColumns::Velocity);
            float& wB = bodyStorage.Get(constraint.BodyB, &BodyColumns::AngularVelocity);

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
}
} // namespace Crumbs2D