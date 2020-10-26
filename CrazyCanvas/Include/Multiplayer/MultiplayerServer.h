#pragma once

#include "Multiplayer/MultiplayerBase.h"

#include "ECS/Systems/Match/ServerFlagSystem.h"
#include "World/Player/Server/PlayerRemoteSystem.h"

class MultiplayerServer : public MultiplayerBase
{
public:
	MultiplayerServer();
	~MultiplayerServer();

protected:
	void Init() override final;
	void TickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;

private:
	ServerFlagSystem* m_pFlagSystem = nullptr;
	PlayerRemoteSystem m_PlayerRemoteSystem;
};