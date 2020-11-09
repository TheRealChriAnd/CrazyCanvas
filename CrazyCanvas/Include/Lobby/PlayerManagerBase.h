#pragma once

#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Time/API/Timestamp.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketJoin.h"
#include "Multiplayer/Packet/PacketLeave.h"
#include "Multiplayer/Packet/PacketPlayerInfo.h"

#include "Lobby/Player.h"

#include "Networking/API/IClient.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Entity.h"

class PlayerManagerBase
{
public:
	DECL_SINGLETON_CLASS(PlayerManagerBase);

	static const Player* GetPlayer(uint64 uid);
	static const Player* GetPlayer(LambdaEngine::IClient* pClient);
	static const Player* GetPlayer(LambdaEngine::Entity entity);
	static const LambdaEngine::THashTable<uint64, Player>& GetPlayers();
	static void RegisterPlayerEntity(uint64 uid, LambdaEngine::Entity entity);

protected:
	static void Init();
	static void Release();

	static Player* GetPlayerNoConst(uint64 uid);
	static Player* GetPlayerNoConst(LambdaEngine::IClient* pClient);
	static Player* GetPlayerNoConst(LambdaEngine::Entity entity);

	static Player* HandlePlayerJoined(uint64 uid, const PacketJoin& packet);
	static void HandlePlayerLeft(uint64 uid);

	static bool UpdatePlayerFromPacket(Player* pPlayer, const PacketPlayerInfo* pPacket);
	static void UpdatePacketFromPlayer(PacketPlayerInfo* pPacket, const Player* pPlayer);

protected:
	static LambdaEngine::THashTable<uint64, Player> s_Players;
	static LambdaEngine::THashTable<LambdaEngine::Entity, uint64> s_PlayerEntityToUID;
};