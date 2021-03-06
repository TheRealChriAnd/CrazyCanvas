#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API SegmentPool
	{
	public:
		SegmentPool(uint16 size);
		~SegmentPool();

#ifdef LAMBDA_CONFIG_DEBUG
		NetworkSegment* RequestFreeSegment(const std::string& borrower);
		bool RequestFreeSegments(uint16 nrOfSegments, TArray<NetworkSegment*>& segmentsReturned, const std::string& borrower);
		void FreeSegment(NetworkSegment* pSegment, const std::string& returner);
		void FreeSegments(TArray<NetworkSegment*>& segments, const std::string& returner);
#else
		
		void FreeSegment(NetworkSegment* pSegment);
		void FreeSegments(TArray<NetworkSegment*>& segments);
#endif
		NetworkSegment* RequestFreeSegment();
		bool RequestFreeSegments(uint16 nrOfSegments, TArray<NetworkSegment*>& segmentsReturned);

		void Reset();

		uint16 GetSize() const;
		uint16 GetFreeSegments() const;

	private:
		void Free(NetworkSegment* pSegment);

	private:
		TArray<NetworkSegment*> m_Segments;
		TArray<NetworkSegment*> m_SegmentsFree;
		SpinLock m_Lock;
	};
}