#pragma once

#include "ClientBase.h"
#include "ISocketTCP.h"
#include "IClientTCPHandler.h"

#define TCP_PING_INTERVAL_NANO_SEC	1000000000
#define TCP_THRESHOLD_NANO_SEC		5000000000

namespace LambdaEngine
{
	class LAMBDA_API ClientTCP : public ClientBase
	{
		friend class SocketFactory;
		friend class ServerTCP;

	public:
		ClientTCP(IClientTCPHandler* handler);
		~ClientTCP();

		/*
		* Connects the client to a given ip-address and port. To connect to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		bool Connect(const std::string& address, uint16 port);

		/*
		* Disconnects the client
		*/
		void Disconnect();

		/*
		* return - true if this instance is on the server side, otherwise false.
		*/
		bool IsServerSide() const;

	protected:
		virtual void OnTransmitterStarted() override;
		virtual void OnReceiverStarted() override;
		virtual void UpdateReceiver(NetworkPacket* packet) override;
		virtual void OnThreadsTerminated() override;
		virtual void OnReleaseRequested() override;
		virtual bool TransmitPacket(NetworkPacket* packet) override;

	private:
		ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket);

		bool Receive(char* buffer, int bytesToRead);
		bool ReceivePacket(NetworkPacket* packet);
		void HandlePacket(NetworkPacket* packet);
		void ResetReceiveTimer();
		void ResetTransmitTimer();
		void Tick(Timestamp timestamp);

	private:
		static ISocketTCP* CreateSocket(const std::string& address, uint16 port);
		static void InitStatic();
		static void TickStatic(Timestamp timestamp);
		static void ReleaseStatic();

	private:
		ISocketTCP* m_pSocket;
		SpinLock m_LockStart;
		IClientTCPHandler* m_pHandler;
		bool m_ServerSide;
		int64 m_TimerReceived;
		int64 m_TimerTransmit;
		uint32 m_NrOfPingTransmitted;
		uint32 m_NrOfPingReceived;

	private:
		static NetworkPacket s_PacketPing;
		static std::set<ClientTCP*>* s_Clients;
		static SpinLock* s_LockClients;
	};
}
