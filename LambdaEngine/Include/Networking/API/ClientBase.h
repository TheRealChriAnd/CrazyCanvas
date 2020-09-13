#pragma once

#include "Networking/API/NetWorker.h"
#include "Networking/API/IClient.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/EProtocol.h"

namespace LambdaEngine
{
	class IClientHandler;
	class ISocket;
	class PacketTransceiverBase;

	struct ClientDesc : public PacketManagerDesc
	{
		IClientHandler* Handler = nullptr;
		EProtocol Protocol		= EProtocol::UDP;
		Timestamp PingTimeout	= Timestamp::Seconds(2);
	};

	class LAMBDA_API ClientBase :
		public NetWorker,
		public IClient,
		protected IPacketListener
	{
		friend class NetworkUtils;

	public:
		DECL_UNIQUE_CLASS(ClientBase);
		virtual ~ClientBase();

		virtual void Disconnect(const std::string& reason) override;
		virtual void Release() override;
		virtual bool IsConnected() override;
		virtual const IPEndPoint& GetEndPoint() const override;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) override;
		virtual EClientState GetState() const override;
		virtual const NetworkStatistics* GetStatistics() const override;
		bool Connect(const IPEndPoint& ipEndPoint);
		virtual bool SendUnreliable(NetworkSegment* packet) override;
		virtual bool SendReliable(NetworkSegment* packet, IPacketListener* listener = nullptr) override;

	protected:
		ClientBase(const ClientDesc& desc);

		void DecodeReceivedPackets();

		virtual bool OnThreadsStarted(std::string& reason) override;
		virtual void RunTransmitter() override;
		virtual void OnThreadsTerminated() override;
		virtual void OnTerminationRequested(const std::string& reason) override;
		virtual void OnReleaseRequested(const std::string& reason) override;

		virtual PacketTransceiverBase* GetTransceiver() = 0;
		virtual ISocket* SetupSocket(std::string& reason) = 0;

	private:
		void TransmitPackets();
		void SendConnect();
		void SendDisconnect();
		void HandleReceivedPacket(NetworkSegment* pPacket);
		void Tick(Timestamp delta);

	protected:
		std::atomic_bool m_SendDisconnectPacket;

	private:
		ISocket* m_pSocket;
		IClientHandler* m_pHandler;
		EClientState m_State;
		SpinLock m_Lock;
		Timestamp m_PingTimeout;

	private:
		static void FixedTickStatic(Timestamp timestamp);

	private:
		static std::set<ClientBase*> s_Clients;
		static SpinLock s_Lock;
	};
}