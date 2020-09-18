#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/ImGuiRenderer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/Camera.h"

#include "Engine/EngineConfig.h"

namespace LambdaEngine
{
	RenderSystem RenderSystem::s_Instance;

	constexpr const uint32 TEMP_DRAW_ARG_MASK = UINT32_MAX;

	bool RenderSystem::Init()
	{
		GraphicsDeviceFeatureDesc deviceFeatures;
		RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
		m_RayTracingEnabled = true;//deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");

		TransformComponents transformComponents;
		transformComponents.Position.Permissions	= R;
		transformComponents.Scale.Permissions		= R;
		transformComponents.Rotation.Permissions	= R;

		// Subscribe on Static Entities & Dynamic Entities
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{RW, MeshComponent::s_TID}},	{&transformComponents}, &m_RenderableEntities, std::bind(&RenderSystem::OnEntityAdded, this, std::placeholders::_1), std::bind(&RenderSystem::OnEntityRemoved, this, std::placeholders::_1)},
				{{{RW, ViewProjectionMatrices::s_TID}}, {&transformComponents}, &m_CameraEntities},
			};
			systemReg.Phase = g_LastPhase;

			RegisterSystem(systemReg);
		}

		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName		= "Renderer Swap Chain";
			swapChainDesc.pWindow		= CommonApplication::Get()->GetActiveWindow().Get();
			swapChainDesc.pQueue		= RenderAPI::GetGraphicsQueue();
			swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width			= 0;
			swapChainDesc.Height		= 0;
			swapChainDesc.BufferCount	= BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount	= 1;
			swapChainDesc.VerticalSync	= false;

			m_SwapChain = RenderAPI::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!m_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			m_ppBackBuffers = DBG_NEW Texture*[BACK_BUFFER_COUNT];
			m_ppBackBufferViews = DBG_NEW TextureView*[BACK_BUFFER_COUNT];

			m_FrameIndex++;
			m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			String prefix = m_RayTracingEnabled ? "RT_" : "";

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "RT_TEST.lrg", IMGUI_ENABLED))
			{
				return false;
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.Name						= "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc	= &renderGraphStructure;
			renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;

			m_pRenderGraph = DBG_NEW RenderGraph(RenderAPI::GetDevice());
			if (!m_pRenderGraph->Init(&renderGraphDesc, m_RequiredDrawArgs))
			{
				LOG_ERROR("[RenderSystem]: Failed to initialize RenderGraph");
				return false;
			}
		}

		//Update RenderGraph with Back Buffer
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				m_ppBackBuffers[v]		= m_SwapChain->GetBuffer(v);
				m_ppBackBufferViews[v]	= m_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_ppBackBufferViews;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		}

		// Per Frame Buffer
		{
			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				BufferDesc perFrameCopyBufferDesc = {};
				perFrameCopyBufferDesc.DebugName		= "Scene Per Frame Staging Buffer " + std::to_string(b);
				perFrameCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				perFrameCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				perFrameCopyBufferDesc.SizeInBytes		= sizeof(PerFrameBuffer);

				m_ppPerFrameStagingBuffers[b] = RenderAPI::GetDevice()->CreateBuffer(&perFrameCopyBufferDesc);
			}

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = RenderAPI::GetDevice()->CreateBuffer(&perFrameBufferDesc);
		}

		//Material Defaults
		{
			for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
			{
				m_FreeMaterialSlots.push(i);
			}

			Texture*		pDefaultColorMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_COLOR_MAP);
			TextureView*	pDefaultColorMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_COLOR_MAP);
			Texture*		pDefaultNormalMap		= ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_NORMAL_MAP);
			TextureView*	pDefaultNormalMapView	= ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_NORMAL_MAP);

			for (uint32 i = 0; i < MAX_UNIQUE_MATERIALS; i++)
			{
				m_ppAlbedoMaps[i]				= pDefaultColorMap;
				m_ppNormalMaps[i]				= pDefaultNormalMap;
				m_ppAmbientOcclusionMaps[i]		= pDefaultColorMap;
				m_ppRoughnessMaps[i]			= pDefaultColorMap;
				m_ppMetallicMaps[i]				= pDefaultColorMap;
				m_ppAlbedoMapViews[i]			= pDefaultColorMapView;
				m_ppNormalMapViews[i]			= pDefaultNormalMapView;
				m_ppAmbientOcclusionMapViews[i]	= pDefaultColorMapView;
				m_ppRoughnessMapViews[i]		= pDefaultColorMapView;
				m_ppMetallicMapViews[i]			= pDefaultColorMapView;
				m_pMaterialInstanceCounts[i]	= 0;
			}
		}

		UpdateBuffers();

		return true;
	}

	bool RenderSystem::Release()
	{
		for (MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
		{
			SAFERELEASE(meshAndInstancesIt->second.pBLAS);
			SAFERELEASE(meshAndInstancesIt->second.pVertexBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pIndexBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pASInstanceBuffer);
			SAFERELEASE(meshAndInstancesIt->second.pRasterInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				SAFERELEASE(meshAndInstancesIt->second.ppASInstanceStagingBuffers[b]);
				SAFERELEASE(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}
		}

		SAFERELEASE(m_pTLAS);
		SAFERELEASE(m_pCompleteInstanceBuffer);

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[b];

			for (DeviceChild* pResource : resourcesToRemove)
			{
				SAFERELEASE(pResource);
			}

			resourcesToRemove.Clear();

			SAFERELEASE(m_ppMaterialParametersStagingBuffers[b]);
			SAFERELEASE(m_ppPerFrameStagingBuffers[b]);
			SAFERELEASE(m_ppStaticStagingInstanceBuffers[b]);
		}				
		SAFERELEASE(m_pMaterialParametersBuffer);
		SAFERELEASE(m_pPerFrameBuffer);

		SAFEDELETE(m_pRenderGraph);

		if (m_SwapChain)
		{
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				SAFERELEASE(m_ppBackBuffers[i]);
				SAFERELEASE(m_ppBackBufferViews[i]);
			}

			SAFEDELETE_ARRAY(m_ppBackBuffers);
			SAFEDELETE_ARRAY(m_ppBackBufferViews);
			m_SwapChain.Reset();
		}

		return true;
	}

	void RenderSystem::Tick(float dt)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<PositionComponent>*	pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>*	pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray<ScaleComponent>*		pScaleComponents	= pECSCore->GetComponentArray<ScaleComponent>();

		for (Entity entity : m_RenderableEntities.GetIDs())
		{
			auto& positionComp	= pPositionComponents->GetData(entity);
			auto& rotationComp	= pRotationComponents->GetData(entity);
			auto& scaleComp		= pScaleComponents->GetData(entity);

			if (positionComp.Dirty || rotationComp.Dirty || scaleComp.Dirty)
			{
				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
				transform *= glm::toMat4(rotationComp.Quaternion);
				transform = glm::scale(transform, scaleComp.Scale);

				UpdateTransform(entity, transform);

				positionComp.Dirty	= false;
				rotationComp.Dirty	= false;
				scaleComp.Dirty		= false;
			}
		}
	}

	bool RenderSystem::Render()
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		CleanBuffers();
		UpdateBuffers();
		UpdateRenderGraph();

		m_pRenderGraph->Update();

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		return true;
	}

	void RenderSystem::SetCamera(const Camera* pCamera)
	{
		m_PerFrameData.CamData = pCamera->GetData();
	}

	CommandList* RenderSystem::AcquireGraphicsCopyCommandList()
	{
		return m_pRenderGraph->AcquireGraphicsCopyCommandList();
	}

	CommandList* RenderSystem::AcquireComputeCopyCommandList()
	{
		return m_pRenderGraph->AcquireGraphicsCopyCommandList();;
	}

	void RenderSystem::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name = name;
		renderGraphDesc.pRenderGraphStructureDesc	= pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;

		m_RequiredDrawArgs.clear();
		if (!m_pRenderGraph->Recreate(&renderGraphDesc, m_RequiredDrawArgs))
		{
			LOG_ERROR("[Renderer]: Failed to set new RenderGraph %s", name.c_str());
		}

		m_DirtyDrawArgs				= m_RequiredDrawArgs;
		m_PerFrameResourceDirty		= true;
		m_MaterialsResourceDirty	= true;
		UpdateRenderGraph();
	}

	void RenderSystem::OnEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		auto& positionComp	= pECSCore->GetComponent<PositionComponent>(entity);
		auto& rotationComp	= pECSCore->GetComponent<RotationComponent>(entity);
		auto& scaleComp		= pECSCore->GetComponent<ScaleComponent>(entity);
		auto& meshComp		= pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
		transform *= glm::toMat4(rotationComp.Quaternion);
		transform = glm::scale(transform, scaleComp.Scale);

		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false);
	}

	void RenderSystem::OnEntityRemoved(Entity entity)
	{
		RemoveEntityInstance(entity);
	}

	void RenderSystem::AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool animated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		uint32 materialSlot = MAX_UNIQUE_MATERIALS;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID		= meshGUID;
		meshKey.IsAnimated		= animated;
		meshKey.EntityID		= entity;

		//Get meshAndInstancesIterator
		{
			meshAndInstancesIt = m_MeshAndInstancesMap.find(meshKey);

			if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
			{
				const Mesh* pMesh = ResourceManager::GetMesh(meshGUID);
				VALIDATE(pMesh != nullptr);

				MeshEntry meshEntry = {};

				//Vertices
				{
					BufferDesc vertexStagingBufferDesc = {};
					vertexStagingBufferDesc.DebugName	= "Vertex Staging Buffer";
					vertexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					vertexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					vertexStagingBufferDesc.SizeInBytes = pMesh->VertexCount * sizeof(Vertex);

					Buffer* pVertexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexStagingBufferDesc);

					void* pMapped = pVertexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pVertexArray, vertexStagingBufferDesc.SizeInBytes);
					pVertexStagingBuffer->Unmap();

					BufferDesc vertexBufferDesc = {};
					vertexBufferDesc.DebugName		= "Vertex Buffer";
					vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					vertexBufferDesc.SizeInBytes	= vertexStagingBufferDesc.SizeInBytes;

					meshEntry.pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);
					meshEntry.VertexCount	= pMesh->VertexCount;

					m_PendingBufferUpdates.PushBack({ pVertexStagingBuffer, 0, meshEntry.pVertexBuffer, 0, vertexBufferDesc.SizeInBytes });
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pVertexStagingBuffer);
				}

				//Indices
				{
					BufferDesc indexStagingBufferDesc = {};
					indexStagingBufferDesc.DebugName	= "Index Staging Buffer";
					indexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					indexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					indexStagingBufferDesc.SizeInBytes	= pMesh->IndexCount * sizeof(uint32);

					Buffer* pIndexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&indexStagingBufferDesc);

					void* pMapped = pIndexStagingBuffer->Map();
					memcpy(pMapped, pMesh->pIndexArray, indexStagingBufferDesc.SizeInBytes);
					pIndexStagingBuffer->Unmap();

					BufferDesc indexBufferDesc = {};
					indexBufferDesc.DebugName		= "Index Buffer";
					indexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					indexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					indexBufferDesc.SizeInBytes		= indexStagingBufferDesc.SizeInBytes;

					meshEntry.pIndexBuffer	= RenderAPI::GetDevice()->CreateBuffer(&indexBufferDesc);
					meshEntry.IndexCount	= pMesh->IndexCount;

					m_PendingBufferUpdates.PushBack({ pIndexStagingBuffer, 0, meshEntry.pIndexBuffer, 0, indexBufferDesc.SizeInBytes });
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pIndexStagingBuffer);
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;

				if (m_RayTracingEnabled)
				{
					meshEntry.ShaderRecord.VertexBufferAddress = meshEntry.pVertexBuffer->GetDeviceAdress();
					meshEntry.ShaderRecord.IndexBufferAddress = meshEntry.pIndexBuffer->GetDeviceAdress();
					m_DirtyBLASs.insert(&meshAndInstancesIt->second);
				}
			}
		}

		//Get Material Slot
		{
			THashTable<uint32, uint32>::iterator materialSlotIt = m_MaterialMap.find(materialGUID);

			//Push new Material if the Material is yet to be registered
			if (materialSlotIt == m_MaterialMap.end())
			{
				const Material* pMaterial = ResourceManager::GetMaterial(materialGUID);
				VALIDATE(pMaterial != nullptr);

				if (!m_FreeMaterialSlots.empty())
				{
					materialSlot = m_FreeMaterialSlots.top();
					m_FreeMaterialSlots.pop();
				}
				else
				{
					for (uint32 m = 0; m < MAX_UNIQUE_MATERIALS; m++)
					{
						if (m_pMaterialInstanceCounts[m] == 0)
						{
							materialSlot = m;
							break;
						}
					}

					if (materialSlot == MAX_UNIQUE_MATERIALS)
					{
						LOG_WARNING("[RenderSystem]: No free Material Slots, Entity will be given a random material");
						materialSlot = 0;
					}
				}

				m_ppAlbedoMaps[materialSlot]				= pMaterial->pAlbedoMap;
				m_ppNormalMaps[materialSlot]				= pMaterial->pNormalMap;
				m_ppAmbientOcclusionMaps[materialSlot]		= pMaterial->pAmbientOcclusionMap;
				m_ppRoughnessMaps[materialSlot]				= pMaterial->pRoughnessMap;
				m_ppMetallicMaps[materialSlot]				= pMaterial->pMetallicMap;
				m_ppAlbedoMapViews[materialSlot]			= pMaterial->pAlbedoMapView;
				m_ppNormalMapViews[materialSlot]			= pMaterial->pNormalMapView;
				m_ppAmbientOcclusionMapViews[materialSlot]	= pMaterial->pAmbientOcclusionMapView;
				m_ppRoughnessMapViews[materialSlot]			= pMaterial->pRoughnessMapView;
				m_ppMetallicMapViews[materialSlot]			= pMaterial->pMetallicMapView;
				m_pMaterialProperties[materialSlot]			= pMaterial->Properties;

				m_MaterialMap.insert({ materialGUID, materialSlot });
				m_MaterialsResourceDirty = true;
			}
			else
			{
				materialSlot = materialSlotIt->second;
			}

			m_pMaterialInstanceCounts[materialSlot]++;
		}

		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.RasterInstances.GetSize();
		m_EntityIDsToInstanceKey[entity] = instanceKey;

		if (m_RayTracingEnabled)
		{
			AccelerationStructureInstance asInstance = {};
			asInstance.Transform		= glm::transpose(transform);
			asInstance.CustomIndex		= materialSlot;
			asInstance.Mask				= 0xFF;
			asInstance.SBTRecordOffset	= 0;
			asInstance.Flags			= FAccelerationStructureInstanceFlag::RAY_TRACING_INSTANCE_FLAG_CULLING_DISABLED;

			meshAndInstancesIt->second.ASInstances.PushBack(asInstance);
			m_TLASDirty = true;
		}

		Instance instance = {};
		instance.Transform			= transform;
		instance.PrevTransform		= transform;
		instance.MaterialSlot		= materialSlot;
		meshAndInstancesIt->second.RasterInstances.PushBack(instance);

		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
		
		//Todo: This needs to come from the Entity in some way
		uint32 drawArgHash = TEMP_DRAW_ARG_MASK;
		if (m_RequiredDrawArgs.count(drawArgHash))
		{
			m_DirtyDrawArgs.insert(drawArgHash);
		}
	}
	
	void RenderSystem::RemoveEntityInstance(Entity entity)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);

		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which does not exist");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);

		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which has no MeshAndInstancesMap entry");
			return;
		}

		const Instance& rasterInstance = meshAndInstancesIt->second.RasterInstances[instanceKeyIt->second.InstanceIndex];

		//Update Material Instance Counts
		{
			m_pMaterialInstanceCounts[rasterInstance.MaterialSlot]--;
		}

		if (m_RayTracingEnabled)
		{
			meshAndInstancesIt->second.ASInstances.Erase(meshAndInstancesIt->second.ASInstances.Begin() + instanceKeyIt->second.InstanceIndex);
			m_TLASDirty = true;
		}

		meshAndInstancesIt->second.RasterInstances.Erase(meshAndInstancesIt->second.RasterInstances.Begin() + instanceKeyIt->second.InstanceIndex);
		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);

		//Unload Mesh, Todo: Should we always do this?
		if (meshAndInstancesIt->second.RasterInstances.IsEmpty())
		{
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pBLAS);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pVertexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pIndexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pASInstanceBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pRasterInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppASInstanceStagingBuffers[b]);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}

			m_DirtyDrawArgs = m_RequiredDrawArgs;
			m_TLASDirty = true;

			m_MeshAndInstancesMap.erase(meshAndInstancesIt);
		}
	}

	void RenderSystem::UpdateTransform(Entity entity, const glm::mat4& transform)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);

		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which is not registered");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);

		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which has no MeshAndInstancesMap entry");
			return;
		}

		if (m_RayTracingEnabled)
		{
			AccelerationStructureInstance* pASInstanceToUpdate = &meshAndInstancesIt->second.ASInstances[instanceKeyIt->second.InstanceIndex];
			pASInstanceToUpdate->Transform = glm::transpose(transform);
			m_TLASDirty = true;
		}

		Instance* pRasterInstanceToUpdate = &meshAndInstancesIt->second.RasterInstances[instanceKeyIt->second.InstanceIndex];
		pRasterInstanceToUpdate->PrevTransform	= pRasterInstanceToUpdate->Transform;
		pRasterInstanceToUpdate->Transform		= transform;
		m_DirtyInstanceBuffers.insert(&meshAndInstancesIt->second);
		m_TLASDirty = true;
	}

	void RenderSystem::UpdateCamera(Entity entity)
	{

	}

	void RenderSystem::CleanBuffers()
	{
		//Todo: Better solution for this, save some Staging Buffers maybe so they don't get recreated all the time?
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_ModFrameIndex];

		for (DeviceChild* pResource : resourcesToRemove)
		{
			SAFERELEASE(pResource);
		}

		resourcesToRemove.Clear();
	}

	void RenderSystem::UpdateBuffers()
	{
		CommandList* pGraphicsCommandList = m_pRenderGraph->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCommandList = m_pRenderGraph->AcquireComputeCopyCommandList();

		//Update Pending Buffer Updates
		{
			ExecutePendingBufferUpdates(pGraphicsCommandList);
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = 0;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		//Update Instance Data
		{
			UpdateInstanceBuffers(pGraphicsCommandList);
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList);
		}

		//Update Acceleration Structures
		if (m_RayTracingEnabled)
		{
			BuildBLASs(pComputeCommandList);
			BuildTLAS(pComputeCommandList);
		}
	}

	void RenderSystem::UpdateRenderGraph()
	{
		//Should we check for Draw Args to be removed here?

		if (!m_DirtyDrawArgs.empty())
		{
			for (uint32 drawArgMask : m_DirtyDrawArgs)
			{
				TArray<DrawArg> drawArgs;
				CreateDrawArgs(drawArgs, drawArgMask);

				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_DRAW_ARGS;
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsMask	= drawArgMask;
				resourceUpdateDesc.ExternalDrawArgsUpdate.pDrawArgs		= drawArgs.GetData();
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsCount	= drawArgs.GetSize();

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
			}

			m_DirtyDrawArgs.clear();
		}

		if (m_PerFrameResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pPerFrameBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_PerFrameResourceDirty = false;
		}

		if (m_MaterialsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pMaterialParametersBuffer;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			std::vector<Sampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, Sampler::GetNearestSampler());

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName								= SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppAlbedoMaps;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppAlbedoMapViews;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName								= SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppNormalMaps;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppNormalMapViews;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc aoMapsUpdateDesc = {};
			aoMapsUpdateDesc.ResourceName									= SCENE_AO_MAPS;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= m_ppAmbientOcclusionMaps;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= m_ppAmbientOcclusionMapViews;
			aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= nearestSamplers.data();

			ResourceUpdateDesc metallicMapsUpdateDesc = {};
			metallicMapsUpdateDesc.ResourceName								= SCENE_METALLIC_MAPS;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_ppMetallicMaps;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_ppMetallicMapViews;
			metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

			ResourceUpdateDesc roughnessMapsUpdateDesc = {};
			roughnessMapsUpdateDesc.ResourceName							= SCENE_ROUGHNESS_MAPS;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppRoughnessMaps;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_ppRoughnessMapViews;
			roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= nearestSamplers.data();

			m_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&aoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&metallicMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&roughnessMapsUpdateDesc);

			m_MaterialsResourceDirty = false;
		}

		if (m_RayTracingEnabled)
		{
			if (m_TLASResourceDirty)
			{
				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_TLAS;
				resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= m_pTLAS;

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

				m_TLASResourceDirty = false;
			}
		}
	}

	void RenderSystem::CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const
	{
		for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
		{
			//Todo: Check Key (or whatever we end up using)
			DrawArg drawArg = {};
			drawArg.pVertexBuffer		= meshAndInstancesIt->second.pVertexBuffer;
			drawArg.VertexBufferSize	= meshAndInstancesIt->second.pVertexBuffer->GetDesc().SizeInBytes;
			drawArg.pIndexBuffer		= meshAndInstancesIt->second.pIndexBuffer;
			drawArg.IndexCount			= meshAndInstancesIt->second.IndexCount;
			drawArg.pInstanceBuffer		= meshAndInstancesIt->second.pRasterInstanceBuffer;
			drawArg.InstanceBufferSize	= meshAndInstancesIt->second.pRasterInstanceBuffer->GetDesc().SizeInBytes;
			drawArg.InstanceCount		= meshAndInstancesIt->second.RasterInstances.GetSize();
			drawArgs.PushBack(drawArg);
		}
	}

	void RenderSystem::ExecutePendingBufferUpdates(CommandList* pCommandList)
	{
		if (!m_PendingBufferUpdates.IsEmpty())
		{
			for (uint32 i = 0; i < m_PendingBufferUpdates.GetSize(); i++)
			{
				const PendingBufferUpdate& pendingUpdate = m_PendingBufferUpdates[i];
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, pendingUpdate.pDstBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			m_PendingBufferUpdates.Clear();
		}
	}

	void RenderSystem::UpdateInstanceBuffers(CommandList* pCommandList)
	{
		for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyInstanceBuffers)
		{
			//AS Instances
			if (m_RayTracingEnabled)
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->ASInstances.GetSize() * sizeof(AccelerationStructureInstance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName	= "AS Instance Staging Buffer";
					bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes	= requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->ASInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pASInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pASInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pASInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pASInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= "AS Instance Buffer";
					bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC | FBufferFlag::BUFFER_FLAG_COPY_DST;
					bufferDesc.SizeInBytes		= requiredBufferSize;

					pDirtyInstanceBufferEntry->pASInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pASInstanceBuffer, 0, requiredBufferSize);
			}

			//Raster Instances
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->RasterInstances.GetSize() * sizeof(Instance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName	= "Raster Instance Staging Buffer";
					bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes	= requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->RasterInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pRasterInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pRasterInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= "Raster Instance Buffer";
					bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
					bufferDesc.SizeInBytes		= requiredBufferSize;

					pDirtyInstanceBufferEntry->pRasterInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pRasterInstanceBuffer, 0, requiredBufferSize);
			}
		}

		m_DirtyInstanceBuffers.clear();
	}

	void RenderSystem::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		Buffer* pPerFrameStagingBuffer = m_ppPerFrameStagingBuffers[m_ModFrameIndex];

		void* pMapped = pPerFrameStagingBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		pPerFrameStagingBuffer->Unmap();

		pCommandList->CopyBuffer(pPerFrameStagingBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void RenderSystem::UpdateMaterialPropertiesBuffer(CommandList* pCommandList)
	{
		if (m_MaterialsPropertiesBufferDirty)
		{
			uint32 requiredBufferSize = sizeof(m_pMaterialProperties);

			Buffer* pStagingBuffer = m_ppStaticStagingInstanceBuffers[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Staging Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppStaticStagingInstanceBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_pMaterialProperties, requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pMaterialParametersBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pMaterialParametersBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				m_pMaterialParametersBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);

			m_MaterialsPropertiesBufferDirty = false;
		}
	}

	void RenderSystem::BuildBLASs(CommandList* pCommandList)
	{
		if (!m_DirtyBLASs.empty())
		{
			for (MeshEntry* pDirtyBLAS : m_DirtyBLASs)
			{
				//We assume that VertexCount/PrimitiveCount does not change and thus do not check if we need to recreate them

				bool update = true;

				if (pDirtyBLAS->pBLAS == nullptr)
				{
					update = false;

					AccelerationStructureGeometryDesc createGeometryDesc = {};
					createGeometryDesc.MaxTriangleCount	= pDirtyBLAS->IndexCount / 3;
					createGeometryDesc.MaxVertexCount	= pDirtyBLAS->VertexCount;
					createGeometryDesc.AllowsTransform	= false;

					AccelerationStructureDesc blasCreateDesc = {};
					blasCreateDesc.DebugName	= "BLAS";
					blasCreateDesc.Type			= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
					blasCreateDesc.Flags		= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
					blasCreateDesc.Geometries	= { createGeometryDesc };

					pDirtyBLAS->pBLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&blasCreateDesc);
				}
				
				BuildBottomLevelAccelerationStructureGeometryDesc buildGeometryDesc = {};
				buildGeometryDesc.pVertexBuffer			= pDirtyBLAS->pVertexBuffer;
				buildGeometryDesc.FirstVertexIndex		= 0;
				buildGeometryDesc.VertexStride			= sizeof(Vertex);
				buildGeometryDesc.pIndexBuffer			= pDirtyBLAS->pIndexBuffer;
				buildGeometryDesc.IndexBufferByteOffset	= 0;
				buildGeometryDesc.TriangleCount			= pDirtyBLAS->IndexCount / 3;

				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= pDirtyBLAS->pBLAS;
				blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				blasBuildDesc.Update					= update;
				blasBuildDesc.Geometries				= { buildGeometryDesc };

				pCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);

				uint64 blasAddress = pDirtyBLAS->pBLAS->GetDeviceAdress();

				for (AccelerationStructureInstance& asInstance : pDirtyBLAS->ASInstances)
				{
					asInstance.AccelerationStructureAddress = blasAddress;
				}
			}

			m_DirtyBLASs.clear();
		}
	}

	void RenderSystem::BuildTLAS(CommandList* pCommandList)
	{
		if (m_TLASDirty)
		{
			m_TLASDirty = false;
			m_CompleteInstanceBufferPendingCopies.Clear();

			uint32 newInstanceCount = 0;

			for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
			{
				uint32 instanceCount = meshAndInstancesIt->second.ASInstances.GetSize();

				PendingBufferUpdate copyToCompleteInstanceBuffer = {};
				copyToCompleteInstanceBuffer.pSrcBuffer		= meshAndInstancesIt->second.pASInstanceBuffer;
				copyToCompleteInstanceBuffer.SrcOffset		= 0;
				copyToCompleteInstanceBuffer.DstOffset		= newInstanceCount * sizeof(AccelerationStructureInstance);
				copyToCompleteInstanceBuffer.SizeInBytes	= instanceCount * sizeof(AccelerationStructureInstance);
				m_CompleteInstanceBufferPendingCopies.PushBack(copyToCompleteInstanceBuffer);

				newInstanceCount += instanceCount;
			}
			
			if (newInstanceCount == 0)
				return;

			uint32 requiredCompleteInstancesBufferSize = newInstanceCount * sizeof(AccelerationStructureInstance);

			if (m_pCompleteInstanceBuffer == nullptr || m_pCompleteInstanceBuffer->GetDesc().SizeInBytes < requiredCompleteInstancesBufferSize)
			{
				if (m_pCompleteInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pCompleteInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Complete Instance Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
				bufferDesc.SizeInBytes	= requiredCompleteInstancesBufferSize;

				m_pCompleteInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			for (const PendingBufferUpdate& pendingUpdate : m_CompleteInstanceBufferPendingCopies)
			{
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, m_pCompleteInstanceBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			bool update = true;

			//Recreate TLAS completely if oldInstanceCount != newInstanceCount
			if (m_MaxInstances < newInstanceCount)
			{
				if (m_pTLAS != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pTLAS);

				m_MaxInstances = newInstanceCount;

				AccelerationStructureGeometryDesc createGeometryDesc = {};
				createGeometryDesc.InstanceCount	= m_MaxInstances;
				m_CreateTLASGeometryDescriptions.PushBack(createGeometryDesc);

				AccelerationStructureDesc createTLASDesc = {};
				createTLASDesc.DebugName	= "TLAS";
				createTLASDesc.Type			= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
				createTLASDesc.Flags		= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				createTLASDesc.Geometries	= m_CreateTLASGeometryDescriptions;

				m_pTLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&createTLASDesc);

				update = false;

				m_TLASResourceDirty = true;
			}

			if (m_pTLAS != nullptr)
			{
				BuildTopLevelAccelerationStructureDesc buildTLASDesc = {};
				buildTLASDesc.pAccelerationStructure	= m_pTLAS;
				buildTLASDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				buildTLASDesc.Update					= update;
				buildTLASDesc.pInstanceBuffer			= m_pCompleteInstanceBuffer;
				buildTLASDesc.InstanceCount				= newInstanceCount;

				pCommandList->BuildTopLevelAccelerationStructure(&buildTLASDesc);
			}
		}
	}
}
