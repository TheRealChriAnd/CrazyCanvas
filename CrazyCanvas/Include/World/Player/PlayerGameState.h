#pragma once

#include "Types.h"
#include "Math/Math.h"

struct PlayerGameState
{
	int32 SimulationTick = -1;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::quat Rotation;
	int8 DeltaForward = 0;
	int8 DeltaLeft = 0;
};

struct GameStateComparator
{
	bool operator() (const PlayerGameState& lhs, const PlayerGameState& rhs) const
	{
		return lhs.SimulationTick < rhs.SimulationTick;
	}
};