#include "Networking/API/NetworkStatistics.h"

#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/TCP/PacketTransceiverTCP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketTransceiverTCP::PacketTransceiverTCP() :
		m_pSocket(nullptr)
	{

	}

	PacketTransceiverTCP::~PacketTransceiverTCP()
	{

	}

	bool PacketTransceiverTCP::Transmit(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& endPoint)
	{
		UNREFERENCED_VARIABLE(endPoint);
		return m_pSocket->Send(pBuffer, bytesToSend, bytesSent);
	}

	bool PacketTransceiverTCP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& endPoint)
	{
		UNREFERENCED_VARIABLE(endPoint);

		static const uint32 headerSize = sizeof(PacketTranscoder::Header);

		if (!ForceReceive(pBuffer, headerSize))
			return false;
		
		PacketTranscoder::Header* header = (PacketTranscoder::Header*)pBuffer;
		if (header->Size > MAXIMUM_SEGMENT_SIZE)
			return false;

		return ForceReceive(pBuffer + headerSize, header->Size - headerSize);
	}

	bool PacketTransceiverTCP::ForceReceive(uint8* pBuffer, uint32 bytesToRead)
	{
		uint32 totalBytesRead = 0;
		int32 bytesRead = 0;
		while (totalBytesRead != bytesToRead)
		{
			if (!m_pSocket->Receive(pBuffer + totalBytesRead, bytesToRead - totalBytesRead, bytesRead))
				return false;
			totalBytesRead += bytesRead;
		}
		return true;
	}

	void PacketTransceiverTCP::OnReceiveEnd(const PacketTranscoder::Header& header, TArray<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		pStatistics->SetLastReceivedSequenceNr(header.Sequence);
		pStatistics->SetLastReceivedAckNr(header.Ack);
		newAcks.PushBack(header.Ack);
	}

	void PacketTransceiverTCP::SetSocket(ISocket* pSocket)
	{
		m_pSocket = (ISocketTCP*)pSocket;
	}
}