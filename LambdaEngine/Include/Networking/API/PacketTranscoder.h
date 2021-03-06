#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	class SegmentPool;

	class LAMBDA_API PacketTranscoder
	{
	public:
#pragma pack(push, 1)
		struct Header
		{
			uint16 Size = 0;
			uint64 Salt = 0;
			uint32 Sequence = 0;
			uint32 Ack = 0;
			uint64 AckBits = 0;
			uint8  Segments = 0;
		};
#pragma pack(pop)

	public:
		DECL_STATIC_CLASS(PacketTranscoder);

		static void EncodeSegments(uint8* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, std::set<NetworkSegment*, NetworkSegmentUIDOrder>& segmentsToEncode, std::set<uint32>& reliableUIDsSent, uint16& bytesWritten, Header* pHeader);
		static bool DecodeSegments(const uint8* buffer, uint16 bufferSize, SegmentPool* pSegmentPool, TArray<NetworkSegment*>& segmentsDecoded, Header* pHeader);

	private:
		static uint16 WriteSegment(uint8* buffer, NetworkSegment* pSegment);
		static uint16 ReadSegment(const uint8* buffer, NetworkSegment* pSegment);
	};
}