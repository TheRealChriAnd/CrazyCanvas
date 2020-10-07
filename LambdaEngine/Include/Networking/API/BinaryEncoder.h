#pragma once

#include "LambdaEngine.h"
#include "Containers/String.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API BinaryEncoder
	{
	public:
		BinaryEncoder(NetworkSegment* packet);
		~BinaryEncoder();

		void WriteInt8(int8 value);
		void WriteUInt8(uint8 value);
		void WriteInt16(int16 value);
		void WriteUInt16(uint16 value);
		void WriteInt32(int32 value);
		void WriteUInt32(uint32 value);
		void WriteInt64(int64 value);
		void WriteUInt64(uint64 value);
		void WriteFloat32(float32 value);
		void WriteFloat64(float64 value);
		void WriteBool(bool value);
		void WriteString(const std::string& value);
		void WriteBuffer(const uint8* pBuffer, uint16 size);

		void WriteVec2(const glm::vec2& value);
		void WriteVec3(const glm::vec3& value);
		void WriteVec4(const glm::vec4& value);

	private:
		NetworkSegment* m_pNetworkPacket;
	};
}