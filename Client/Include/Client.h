#pragma once

#include "Game/Game.h"

#include "Rendering/RenderGraphTypes.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphEditor.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/IPacketListener.h"
#include "Networking/API/IClientHandler.h"

namespace LambdaEngine
{
	class RenderGraph;
	class Renderer;
	class IClient;
	class NetworkSegment;
	class ClientBase;
}

class Client :
	public LambdaEngine::Game,
	public LambdaEngine::ApplicationEventHandler,
	public LambdaEngine::IPacketListener,
	public LambdaEngine::IClientHandler
{
public:
	Client();
	~Client();

	virtual void OnConnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnConnected(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnected(LambdaEngine::IClient* pClient) override;
	virtual void OnPacketReceived(LambdaEngine::IClient* pClient, LambdaEngine::NetworkSegment* pPacket) override;
	virtual void OnServerFull(LambdaEngine::IClient* pClient) override;
	virtual void OnClientReleased(LambdaEngine::IClient* pClient) override;

	virtual void OnPacketDelivered(LambdaEngine::NetworkSegment* pPacket) override;
	virtual void OnPacketResent(LambdaEngine::NetworkSegment* pPacket, uint8 tries) override;
	virtual void OnPacketMaxTriesReached(LambdaEngine::NetworkSegment* pPacket, uint8 tries) override;

	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	LambdaEngine::ClientBase* m_pClient;

	LambdaEngine::RenderGraphEditor* m_pRenderGraphEditor = nullptr;
	LambdaEngine::RenderGraph* m_pRenderGraph = nullptr;
	LambdaEngine::Renderer* m_pRenderer = nullptr;
	LambdaEngine::Scene* m_pScene = nullptr;
};
