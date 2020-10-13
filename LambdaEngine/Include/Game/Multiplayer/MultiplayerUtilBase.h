#pragma once

#include "Types.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/PacketFunction.h"

namespace LambdaEngine
{
	class IClient;

	class MultiplayerUtilBase
	{
		friend class ClientSystem;
		friend class ServerSystem;

	public:
		DECL_ABSTRACT_CLASS(MultiplayerUtilBase);

		virtual Entity GetEntity(int32 networkUID) const = 0;
		virtual void RegisterEntity(Entity entity, int32 networkUID) = 0;
		virtual uint64 GetSaltAsUID(IClient* pClient) = 0;
	};
}