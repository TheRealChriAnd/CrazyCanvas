#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClientUDP.h"
#include "Networking/API/PacketManager.h"

namespace LambdaEngine
{
	class ServerUDP;
	class IClientUDPHandler;

	class LAMBDA_API ClientUDPRemote : public IClientUDP
	{
		friend class ServerUDP;
		
	public:
		~ClientUDPRemote();

		virtual void Disconnect() override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual bool SendUnreliable(NetworkPacket* packet) override;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkPacket* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;

	protected:
		ClientUDPRemote(uint16 packets, const IPEndPoint& ipEndPoint, ServerUDP* pServer);

	private:
		PacketManager* GetPacketManager();
		void OnDataReceived(const char* data, int32 size);
		void SendPackets();
		bool HandleReceivedPacket(NetworkPacket* pPacket);

	private:
		ServerUDP* m_pServer;
		IPEndPoint m_IPEndPoint;
		PacketManager m_PacketManager;
		SpinLock m_Lock;
		NetworkPacket* m_pPackets[32];
		IClientUDPHandler* m_pHandler;
		EClientState m_State;
		std::atomic_bool m_Release;
		bool m_DisconnectedByRemote;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}