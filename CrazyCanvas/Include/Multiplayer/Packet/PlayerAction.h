#pragma once

#include "Multiplayer/Packet/Packet.h"
#include "Math/Math.h"

#pragma pack(push, 1)
struct PlayerAction : Packet
{
	glm::quat Rotation;
	int8 DeltaForward = 0;
	int8 DeltaLeft = 0;
};
#pragma pack(pop)