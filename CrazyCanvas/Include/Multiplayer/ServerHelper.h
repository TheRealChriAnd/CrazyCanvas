#pragma once

#include "LambdaEngine.h"

#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Networking/API/IPacketListener.h"

#include "Lobby/Player.h"

class ServerHelper
{
	DECL_STATIC_CLASS(ServerHelper);

public:
	template<class T>
	static bool Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener = nullptr);

	template<class T>
	static bool SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener = nullptr, LambdaEngine::IClient* pExcludeClient = nullptr);

	static void SetMaxClients(uint8 clients);
	static void SetIgnoreNewClients(bool ignore);

	static void DisconnectPlayer(const Player* pPlayer, const LambdaEngine::String& reason);

	//static class LambdaEngine::ClientRemoteBase* GetClient(uint64 uid);
};

template<class T>
bool ServerHelper::Send(LambdaEngine::IClient* pClient, const T& packet, LambdaEngine::IPacketListener* pListener)
{
	ASSERT_MSG(T::Type() != 0, "Packet type not registered!")
	return pClient->SendReliableStruct<T>(packet, T::Type(), pListener);
}

template<class T>
bool ServerHelper::SendBroadcast(const T& packet, LambdaEngine::IPacketListener* pListener, LambdaEngine::IClient* pExcludeClient)
{
	ASSERT_MSG(T::Type() != 0, "Packet type not registered!")
	LambdaEngine::ServerBase* pServer = LambdaEngine::ServerSystem::GetInstance().GetServer();
	return pServer->SendReliableStructBroadcast<T>(packet, T::Type(), pListener, pExcludeClient);
}