#include "Networking/API/PacketManagerBase.h"

#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiverBase.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManagerBase::PacketManagerBase(const PacketManagerDesc& desc) :
		m_SegmentPool(desc.PoolSize),
		m_QueueIndex(0)
	{

	}

	uint32 PacketManagerBase::EnqueueSegmentReliable(NetworkSegment* pSegment, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		uint32 reliableUID = m_Statistics.RegisterReliableSegmentSent();
		uint32 UID = EnqueueSegment(pSegment, reliableUID);
		m_SegmentsWaitingForAck.insert({ reliableUID, SegmentInfo{ pSegment, pListener, EngineLoop::GetTimeSinceStart()} });
		return UID;
	}

	uint32 PacketManagerBase::EnqueueSegmentUnreliable(NetworkSegment* pSegment)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		return EnqueueSegment(pSegment, 0);
	}

	uint32 PacketManagerBase::EnqueueSegment(NetworkSegment* pSegment, uint32 reliableUID)
	{
		if(pSegment->GetType() < 999)
			ASSERT(pSegment->GetBufferSize() > 0);

		pSegment->GetHeader().UID = m_Statistics.RegisterUniqueSegment();
		pSegment->GetHeader().ReliableUID = reliableUID;
		InsertSegment(pSegment);
		return pSegment->GetHeader().UID;
	}

	void PacketManagerBase::InsertSegment(NetworkSegment* pSegment)
	{
		m_SegmentsToSend[m_QueueIndex].insert(pSegment);
	}

	void PacketManagerBase::Flush(PacketTransceiverBase* pTransceiver)
	{
		std::set<NetworkSegment*, NetworkSegmentUIDOrder>& segments = m_SegmentsToSend[m_QueueIndex];

		{
			std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
			m_QueueIndex = (m_QueueIndex + 1) % 2;
		}

		Timestamp timestamp = EngineLoop::GetTimeSinceStart();

		while (!segments.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_SegmentPool, segments, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
				bundle.Timestamp = timestamp;

				std::scoped_lock<SpinLock> lock(m_LockBundles);
				m_Bundles.insert({ bundleUID, bundle });
			}
		}
	}

	void PacketManagerBase::QueryEnd(TArray<NetworkSegment*>& segmentsReceived)
	{
		m_SegmentPool.FreeSegments(segmentsReceived);
	}

	void PacketManagerBase::DeleteOldBundles()
	{
		Timestamp maxAllowedTime = m_Statistics.GetPing() * 100;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		TArray<uint32> bundlesToDelete;

		std::scoped_lock<SpinLock> lock(m_LockBundles);
		for (auto& pair : m_Bundles)
		{
			if (currentTime - pair.second.Timestamp > maxAllowedTime)
			{
				bundlesToDelete.PushBack(pair.first);
			}
		}

		for (uint32 UID : bundlesToDelete)
			m_Bundles.erase(UID);
	}

	void PacketManagerBase::Tick(Timestamp delta)
	{
		static const Timestamp delay = Timestamp::Seconds(1);

		m_Timer += delta;
		if (m_Timer >= delay)
		{
			m_Timer -= delay;
			DeleteOldBundles();
		}
	}

	SegmentPool* PacketManagerBase::GetSegmentPool()
	{
		return &m_SegmentPool;
	}

	const NetworkStatistics* PacketManagerBase::GetStatistics() const
	{
		return &m_Statistics;
	}

	const IPEndPoint& PacketManagerBase::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void PacketManagerBase::SetEndPoint(const IPEndPoint& ipEndPoint)
	{
		m_IPEndPoint = ipEndPoint;
	}

	void PacketManagerBase::Reset()
	{
		std::scoped_lock<SpinLock> lock1(m_LockSegmentsToSend);
		std::scoped_lock<SpinLock> lock2(m_LockBundles);
		m_SegmentsToSend[0] = {};
		m_SegmentsToSend[1] = {};
		m_SegmentsWaitingForAck.clear();
		m_Bundles.clear();

		m_SegmentPool.Reset();
		m_Statistics.Reset();
		m_QueueIndex = 0;
	}

	bool PacketManagerBase::QueryBegin(PacketTransceiverBase* pTransceiver, TArray<NetworkSegment*>& segmentsReturned)
	{
		TArray<NetworkSegment*> segments;
		TArray<uint32> acks;

		if (!pTransceiver->ReceiveEnd(&m_SegmentPool, segments, acks, &m_Statistics))
			return false;

		segmentsReturned.Clear();
		segmentsReturned.Reserve(segments.GetSize());

		//LOG_MESSAGE("PING %fms", GetStatistics()->GetPing().AsMilliSeconds());

		HandleAcks(acks);
		return FindSegmentsToReturn(segments, segmentsReturned);
	}

	/*
	* Finds packets that have been sent erlier and are now acked.
	* Notifies the listener that the packet was succesfully delivered.
	* Removes the packet and returns it to the pool.
	*/
	void PacketManagerBase::HandleAcks(const TArray<uint32>& ackedPackets)
	{
		TArray<uint32> ackedReliableUIDs;
		GetReliableUIDsFromAckedPackets(ackedPackets, ackedReliableUIDs);

		TArray<SegmentInfo> segmentsAcked;
		GetReliableSegmentInfosFromUIDs(ackedReliableUIDs, segmentsAcked);

		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(segmentsAcked.GetSize());

		for (SegmentInfo& segmentInfo : segmentsAcked)
		{
			if (segmentInfo.Listener)
			{
				segmentInfo.Listener->OnPacketDelivered(segmentInfo.Segment);
			}
			packetsToFree.PushBack(segmentInfo.Segment);
		}

		m_SegmentPool.FreeSegments(packetsToFree);
	}

	/*
	* Finds all Reliable Segment UIDs corresponding to the acks from physical packets
	*/
	void PacketManagerBase::GetReliableUIDsFromAckedPackets(const TArray<uint32>& ackedPackets, TArray<uint32>& ackedReliableUIDs)
	{
		ackedReliableUIDs.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockBundles);

		Timestamp timestamp = 0;
		uint8 timestamps = 0;
		for (uint32 ack : ackedPackets)
		{
			auto iterator = m_Bundles.find(ack);
			if (iterator != m_Bundles.end())
			{
				Bundle& bundle = iterator->second;
				for (uint32 reliableUID : bundle.ReliableUIDs)
					ackedReliableUIDs.PushBack(reliableUID);

				timestamp += bundle.Timestamp;
				m_Bundles.erase(iterator);
				timestamps++;
			}
		}

		if (timestamp != 0)
		{
			timestamp /= timestamps;
			RegisterRTT(EngineLoop::GetTimeSinceStart() - timestamp);
		}
	}

	void PacketManagerBase::GetReliableSegmentInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<SegmentInfo>& ackedReliableSegments)
	{
		ackedReliableSegments.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);

		for (uint32 reliableUID : ackedReliableUIDs)
		{
			auto iterator = m_SegmentsWaitingForAck.find(reliableUID);
			if (iterator != m_SegmentsWaitingForAck.end())
			{
				ackedReliableSegments.PushBack(iterator->second);
				m_SegmentsWaitingForAck.erase(iterator);
			}
		}
	}

	void PacketManagerBase::RegisterRTT(Timestamp rtt)
	{
		static const double scalar1 = 1.0f / 5.0f;
		static const double scalar2 = 1.0f - scalar1;
		m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
	}
}