#pragma once

#include "Multiplayer/Packet/Packet.h"
#include "Math/Math.h"

#pragma pack(push, 1)
struct PlayerActionResponse : Packet
{
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::quat Rotation;
};
#pragma pack(pop)