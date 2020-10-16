#pragma once

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine 
{
	class Sampler;
	class Texture;
	class Buffer;
	class CommandList;
	class RenderGraph;
	class DeviceChild;

	struct ParticleChunk
	{
		uint32 Offset;
		uint32 Size;
	};

	struct ParticleEmitterInstance
	{
		bool			OneTime = false;
		glm::vec3		Position;
		glm::quat		Rotation;
		float			Angle = 90.f;
		float			Velocity;
		float			Acceleration;
		float			ElapTime = 0.f;
		float			LifeTime;
		float			ParticleRadius;
		uint32			IndirectDataIndex = 0;
		ParticleChunk	ParticleChunk;
	};

	struct SParticle
	{
		glm::mat4 Transform;
		glm::vec4 Color;
		glm::vec3 StartPosition;
		float CurrentLife;
		glm::vec3 Velocity;
		float LifeTime;
		glm::vec3 StartVelocity;
		float Radius;
		glm::vec3 Acceleration;
		uint32 AtlasIndex;
		uint32 TileIndex;
		uint32 AnimationCount;
		float Padding0;
		float Padding1;
	};

	struct SAtlasInfo
	{
		float	TileFactorX = 0.f;
		float	TileFactorY = 0.f;
		uint32	RowCount	= 0;
		uint32	ColCount	= 0;
	};

	struct IndirectData
	{
		uint32	IndexCount		= 0;
		uint32	InstanceCount	= 0;
		uint32	FirstIndex		= 0;
		int32	VertexOffset	= 0;
		uint32	FirstInstance	= 0;
	};

	class ParticleManager
	{
	public:
		ParticleManager() = default;
		~ParticleManager() = default;

		void Init(uint32 maxParticleCapacity);
		void Release();

		void Tick(Timestamp deltaTime, uint32 modFrameIndex);

		void OnEmitterEntityAdded(Entity entity);
		void OnEmitterEntityRemoved(Entity entity);

		uint32 GetParticleCount() const { return m_Particles.GetSize();  }
		uint32 GetActiveEmitterCount() const { return m_IndirectData.GetSize();  }
		uint32 GetMaxParticleCount() const { return m_MaxParticleCount; }

		TArray<Texture*>& GetAtlasTextures() { return m_AtlasTextures; }
		TArray<Sampler*>& GetAtlasSamplers() { return m_AtlasSamplers; }

		bool UpdateBuffers(CommandList* pCommandList);
		bool UpdateResources(RenderGraph* pRendergraph);
	private:
		bool CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance);
		bool CopyDataToBuffer(CommandList* pCommandList, void* data, uint64 size, Buffer** pStagingBuffers, Buffer** pBuffer, FBufferFlags flags, const String& name);

		void CleanBuffers();
	private:
		uint32						m_MaxParticleCount;
		uint32						m_ModFrameIndex;

		bool						m_DirtyParticleBuffer	= false;
		bool						m_DirtyVertexBuffer		= false;
		bool						m_DirtyIndexBuffer		= false;
		bool						m_DirtyIndirectBuffer	= false;
		bool						m_DirtyAtlasDataBuffer	= false;

		Buffer*						m_ppIndirectStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pIndirectBuffer = nullptr;

		Buffer*						m_ppVertexStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pVertexBuffer = nullptr;

		Buffer*						m_ppIndexStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pIndexBuffer = nullptr;

		Buffer*						m_ppParticleStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pParticleBuffer = nullptr;

		Buffer*						m_ppAtlasDataStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pAtlasDataBuffer = nullptr;

		TArray<DeviceChild*>		m_ResourcesToRemove[BACK_BUFFER_COUNT];

		TArray<SParticle>			m_Particles;
		TArray<IndirectData>		m_IndirectData;
		TArray<ParticleChunk>		m_FreeParticleChunks;
		TArray<SAtlasInfo>			m_AtlasInfoData;

		THashTable<GUID_Lambda, SAtlasInfo>	m_AtlasResources;
		TArray<Texture*>			m_AtlasTextures;
		TArray<Sampler*>			m_AtlasSamplers;

		THashTable<Entity, ParticleEmitterInstance>	m_ActiveEmitters;
		THashTable<Entity, ParticleEmitterInstance>	m_SleepingEmitters;
	};
}