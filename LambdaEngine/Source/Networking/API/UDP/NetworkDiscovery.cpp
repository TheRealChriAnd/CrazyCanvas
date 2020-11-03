#include "Networking/API/UDP/NetworkDiscovery.h"
#include "Networking/API/UDP/INetworkDiscoveryServer.h"
#include "Networking/API/UDP/INetworkDiscoveryClient.h"
#include "Networking/API/UDP/ServerNetworkDiscovery.h"
#include "Networking/API/UDP/ClientNetworkDiscovery.h"
#include "Networking/API/PlatformNetworkUtils.h"


namespace LambdaEngine
{
	ServerNetworkDiscovery* NetworkDiscovery::s_pServer = nullptr;
	ClientNetworkDiscovery* NetworkDiscovery::s_pClient = nullptr;
	SpinLock NetworkDiscovery::s_Lock;
	TSet<IPEndPoint> NetworkDiscovery::s_EndPoints;

	bool NetworkDiscovery::EnableServer(const String& nameOfGame, uint16 portOfGameServer, INetworkDiscoveryServer* pHandler, uint16 portOfBroadcastServer)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		if (!s_pServer)
		{
			s_pServer = DBG_NEW ServerNetworkDiscovery();
			return s_pServer->Start(IPEndPoint(IPAddress::ANY, portOfBroadcastServer), nameOfGame, portOfGameServer, pHandler);
		}
		return false;
	}

	void NetworkDiscovery::DisableServer()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		if (s_pServer)
		{
			s_pServer->Release();
			s_pServer = nullptr;
		}
	}

	bool NetworkDiscovery::IsServerEnabled()
	{
		return s_pServer;
	}

	bool NetworkDiscovery::EnableClient(const String& nameOfGame, INetworkDiscoveryClient* pHandler, uint16 portOfBroadcastServer, Timestamp searchInterval)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		if (!s_pClient)
		{
			s_pClient = DBG_NEW ClientNetworkDiscovery();
			s_EndPoints.insert(IPEndPoint(IPAddress::BROADCAST, portOfBroadcastServer));
			return s_pClient->Connect(&s_EndPoints, &s_Lock, nameOfGame, pHandler, searchInterval);
		}
		return false;
	}

	void NetworkDiscovery::DisableClient()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		if (s_pClient)
		{
			s_pClient->Release();
			s_pClient = nullptr;
		}
	}

	bool NetworkDiscovery::IsClientEnabled()
	{
		return s_pClient;
	}

	void NetworkDiscovery::AddTarget(IPAddress* pAddress, uint16 portOfBroadcastServer)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_EndPoints.insert(IPEndPoint(pAddress, portOfBroadcastServer));
	}

	void NetworkDiscovery::RemoveTarget(IPAddress* pAddress, uint16 portOfBroadcastServer)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_EndPoints.erase(IPEndPoint(pAddress, portOfBroadcastServer));
	}

	const TSet<IPEndPoint>& NetworkDiscovery::GetTargets()
	{
		return s_EndPoints;
	}

	void NetworkDiscovery::FixedTickStatic(Timestamp delta)
	{
		if (s_pClient)
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			if (s_pClient)
			{
				s_pClient->FixedTick(delta);
			}
		}
	}

	void NetworkDiscovery::ReleaseStatic()
	{
		DisableServer();
		DisableClient();
	}
}
