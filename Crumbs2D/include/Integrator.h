#pragma once
#include "BodyStorage.h"
namespace Crumbs2D
{
struct PhysicsSettings;

namespace Integrator
{
void IntegrateVelocities(BodyStorage& bodies, const PhysicsSettings& settings, float dt);

void IntegratePositions(BodyStorage& bodies, float dt);
} // namespace Integrator
} // namespace Crumbs2D