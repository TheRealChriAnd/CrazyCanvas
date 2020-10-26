#include "Multiplayer/MultiplayerServer.h"

MultiplayerServer::MultiplayerServer() : 
	m_PlayerRemoteSystem(),
	m_pFlagSystem(nullptr)
{
}

MultiplayerServer::~MultiplayerServer()
{
	SAFEDELETE(m_pFlagSystem);
}

void MultiplayerServer::Init()
{
	m_pFlagSystem = DBG_NEW ServerFlagSystem();
	m_pFlagSystem->Init();

	m_WeaponSystem.Init();
	m_PlayerRemoteSystem.Init();
}

void MultiplayerServer::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void MultiplayerServer::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_pFlagSystem->FixedTick(deltaTime);
	m_WeaponSystem.FixedTick(deltaTime);
	m_PlayerRemoteSystem.FixedTickMainThread(deltaTime);
}
