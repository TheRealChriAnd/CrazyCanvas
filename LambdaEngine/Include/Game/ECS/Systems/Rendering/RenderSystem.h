#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Resources/Mesh.h"
#include "Resources/Material.h"

#include "Utilities/HashUtilities.h"

#include "Containers/String.h"
#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/TStack.h"
#include "Containers/IDVector.h"

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/AccelerationStructure.h"

#include "Rendering/RenderGraphTypes.h"

namespace LambdaEngine
{
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	class PhysicsRenderer;

	struct RenderGraphStructureDesc;

	class LAMBDA_API RenderSystem : public System
	{
		struct Instance
		{
			glm::mat4	Transform		= glm::mat4(1.0f);
			glm::mat4	PrevTransform	= glm::mat4(1.0f);
			uint32		MaterialSlot	= 0;
			uint32		Padding0;
			uint32		Padding1;
			uint32		Padding2;
		};

		struct MeshKey
		{
			GUID_Lambda		MeshGUID;
			bool			IsAnimated;
			Entity			EntityID;
			mutable size_t	Hash = 0;

			size_t GetHash() const
			{
				if (Hash == 0)
				{
					Hash = std::hash<GUID_Lambda>()(MeshGUID);

					if (IsAnimated) HashCombine<GUID_Lambda>(Hash, (GUID_Lambda)EntityID);
				}

				return Hash;
			}

			bool operator==(const MeshKey& other) const
			{
				if (MeshGUID != other.MeshGUID)
					return false;

				if (IsAnimated)
				{
					if (!other.IsAnimated || EntityID != other.EntityID)
						return false;
				}

				return true;
			}
		};

		struct MeshKeyHasher
		{
			size_t operator()(const MeshKey& key) const
			{
				return key.GetHash();
			}
		};

		struct MeshEntry
		{
			AccelerationStructure* pBLAS		= nullptr;
			SBTRecord ShaderRecord			= {};

			Buffer* pVertexBuffer			= nullptr;
			uint32	VertexCount				= 0;
			Buffer* pIndexBuffer			= nullptr;
			uint32	IndexCount				= 0;

			
			Buffer* pASInstanceBuffer		= nullptr;
			Buffer* ppASInstanceStagingBuffers[BACK_BUFFER_COUNT];
			TArray<AccelerationStructureInstance> ASInstances;

			Buffer* pRasterInstanceBuffer			= nullptr;
			Buffer* ppRasterInstanceStagingBuffers[BACK_BUFFER_COUNT];
			TArray<Instance> RasterInstances;

			TArray<Entity> EntityIDs;
		};

		struct InstanceKey
		{
			MeshKey MeshKey;
			uint32	InstanceIndex = 0;
		};

		struct PendingBufferUpdate
		{
			Buffer* pSrcBuffer	= nullptr;
			uint64	SrcOffset	= 0;
			Buffer* pDstBuffer	= nullptr;
			uint64	DstOffset	= 0;
			uint64	SizeInBytes	= 0;
		};

		using MeshAndInstancesMap	= THashTable<MeshKey, MeshEntry, MeshKeyHasher>;
		using MaterialMap			= THashTable<GUID_Lambda, uint32>;

		struct CameraData
		{
			glm::mat4 Projection		= glm::mat4(1.0f);
			glm::mat4 View				= glm::mat4(1.0f);
			glm::mat4 PrevProjection	= glm::mat4(1.0f);
			glm::mat4 PrevView			= glm::mat4(1.0f);
			glm::mat4 ViewInv			= glm::mat4(1.0f);
			glm::mat4 ProjectionInv		= glm::mat4(1.0f);
			glm::vec4 Position			= glm::vec4(0.0f);
			glm::vec4 Right				= glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec4 Up				= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec2 Jitter			= glm::vec2(0.0f);
		};

		struct PerFrameBuffer
		{
			CameraData CamData;

			uint32 FrameIndex;
			uint32 RandomSeed;
		};

		struct PointLight
		{
			glm::vec4	ColorIntensity	= glm::vec4(1.0f);
			glm::vec3	Position		= glm::vec3(0.0f);
			uint32		Padding0;
		};

		struct LightBuffer
		{
			glm::vec4	ColorIntensity	= glm::vec4(0.0f);
			glm::vec3	Direction		= glm::vec3(1.0f);
			uint32		PointLightCount = 0U;
			// PointLight PointLights[] unbounded
		};

	public:
		DECL_REMOVE_COPY(RenderSystem);
		DECL_REMOVE_MOVE(RenderSystem);
		~RenderSystem() = default;

		bool Init();
		bool Release();

		void Tick(Timestamp deltaTime) override final;

		bool Render();

		void SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc);

		RenderGraph*	GetRenderGraph()			{ return m_pRenderGraph;	}
		uint64			GetFrameIndex() const	 	{ return m_FrameIndex; }
		uint64			GetModFrameIndex() const	{ return m_ModFrameIndex;	}
		uint32			GetBufferIndex() const	 	{ return m_BackBufferIndex; }

	public:
		static RenderSystem& GetInstance() { return s_Instance; }

	private:
		RenderSystem() = default;

		void OnEntityAdded(Entity entity);
		void OnEntityRemoved(Entity entity);

		void AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool animated);
		
		void OnDirectionalEntityAdded(Entity entity);
		void OnPointLightEntityAdded(Entity entity);

		void OnDirectionalEntityRemoved(Entity entity);
		void OnPointLightEntityRemoved(Entity entity);

		void RemoveEntityInstance(Entity entity);
		void UpdateDirectionalLight(Entity entity, glm::vec4& colorIntensity, glm::quat& direction);
		void UpdatePointLight(Entity entity, const glm::vec3& position, glm::vec4& colorIntensity);
		void UpdateTransform(Entity entity, const glm::mat4& transform);
		void UpdateCamera(Entity entity);

		void CleanBuffers();
		void CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const;

		void UpdateBuffers();
		void ExecutePendingBufferUpdates(CommandList* pCommandList);
		void UpdatePerFrameBuffer(CommandList* pCommandList);
		void UpdateRasterInstanceBuffers(CommandList* pCommandList);
		void UpdateMaterialPropertiesBuffer(CommandList* pCommandList);
		void UpdateLightsBuffer(CommandList* pCommandList);
		void UpdateShaderRecords();
		void BuildBLASs(CommandList* pCommandList);
		void UpdateASInstanceBuffers(CommandList* pCommandList);
		void BuildTLAS(CommandList* pCommandList);

		void UpdateRenderGraph();

	private:

		IDVector				m_DirectionalLightEntities;
		IDVector				m_PointLightEntities;
		IDVector				m_RenderableEntities;
		IDVector				m_CameraEntities;

		PhysicsRenderer*		m_pPhysicsRenderer	= nullptr;

		TSharedRef<SwapChain>	m_SwapChain			= nullptr;
		Texture**				m_ppBackBuffers		= nullptr;
		TextureView**			m_ppBackBufferViews	= nullptr;
		RenderGraph*			m_pRenderGraph		= nullptr;
		uint64					m_FrameIndex		= 0;
		uint64					m_ModFrameIndex		= 0;
		uint32					m_BackBufferIndex	= 0;
		bool					m_RayTracingEnabled	= false;

		bool						m_LightsDirty			= true;
		bool						m_LightsResourceDirty	= false;
		bool						m_DirectionalExist		= false;
		LightBuffer					m_LightBufferData;
		THashTable<Entity, uint32>	m_EntityToPointLight;
		THashTable<uint32, Entity>	m_PointLightToEntity;
		TArray<PointLight>			m_PointLights;

		//Data Supplied to the RenderGraph
		MeshAndInstancesMap				m_MeshAndInstancesMap;
		MaterialMap						m_MaterialMap;
		THashTable<Entity, InstanceKey> m_EntityIDsToInstanceKey;

		//Materials
		Texture*			m_ppAlbedoMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppNormalMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppAmbientOcclusionMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppRoughnessMaps[MAX_UNIQUE_MATERIALS];
		Texture*			m_ppMetallicMaps[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppAlbedoMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppNormalMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppAmbientOcclusionMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppRoughnessMapViews[MAX_UNIQUE_MATERIALS];
		TextureView*		m_ppMetallicMapViews[MAX_UNIQUE_MATERIALS];
		MaterialProperties	m_pMaterialProperties[MAX_UNIQUE_MATERIALS];
		uint32				m_pMaterialInstanceCounts[MAX_UNIQUE_MATERIALS];
		Buffer*				m_ppMaterialParametersStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*				m_pMaterialParametersBuffer				= nullptr;
		TStack<uint32>		m_FreeMaterialSlots;

		//Per Frame
		PerFrameBuffer		m_PerFrameData;


		Buffer*				m_ppLightsStagingBuffer[BACK_BUFFER_COUNT] = {nullptr};
		Buffer*				m_pLightsBuffer								= nullptr;
		Buffer*				m_ppPerFrameStagingBuffers[BACK_BUFFER_COUNT];
		Buffer*				m_pPerFrameBuffer			= nullptr;

		//Draw Args
		TSet<uint32>		m_RequiredDrawArgs;

		//Ray Tracing
		Buffer*					m_ppStaticStagingInstanceBuffers[BACK_BUFFER_COUNT];
		Buffer*					m_pCompleteInstanceBuffer		= nullptr;
		uint32					m_MaxInstances					= 0;
		AccelerationStructure*	m_pTLAS							= nullptr;
		TArray<PendingBufferUpdate> m_CompleteInstanceBufferPendingCopies;
		TArray<SBTRecord> m_SBTRecords;

		//Pending/Dirty
		bool						m_SBTRecordsDirty					= true;
		bool						m_RenderGraphSBTRecordsDirty		= true;
		bool						m_MaterialsPropertiesBufferDirty	= true;
		bool						m_MaterialsResourceDirty			= true;
		bool						m_PerFrameResourceDirty				= true;
		TSet<uint32>				m_DirtyDrawArgs;
		TSet<MeshEntry*>			m_DirtyASInstanceBuffers;
		TSet<MeshEntry*>			m_DirtyRasterInstanceBuffers;
		TSet<MeshEntry*>			m_DirtyBLASs;
		bool						m_TLASDirty							= true;
		bool						m_TLASResourceDirty					= false;
		TArray<PendingBufferUpdate> m_PendingBufferUpdates;
		TArray<DeviceChild*>		m_ResourcesToRemove[BACK_BUFFER_COUNT];

	private:
		static RenderSystem		s_Instance;
	};
}