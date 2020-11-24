#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/MatchServer.h"

#include "MeshPaint/MeshPaintHandler.h"

#include "Lobby/PlayerManagerServer.h"

#include "Resources/ResourceManager.h"

// Used for health buffer calculated in a compute shader
#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"

#include <mutex>

// TEMP REMOVE
#include "Input/API/Input.h"

/*
* HealthSystemServer
*/

HealthSystemServer::~HealthSystemServer()
{
	using namespace LambdaEngine;

	// SAFERELEASE(m_CopyFence);
	// SAFERELEASE(m_CommandList);
	// SAFERELEASE(m_CommandAllocator);
	// SAFERELEASE(m_HealthBuffer);
	// SAFERELEASE(m_CopyBuffer);
	// SAFERELEASE(m_VertexCountBuffer);

	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HealthSystemServer::OnProjectileHit);
}

void HealthSystemServer::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	if (!m_HitInfoToProcess.IsEmpty())
	{
		RenderSystem::GetInstance().GetRenderGraph()->TriggerRenderStage("COMPUTE_HEALTH");
	}

	// TEMP REMOVE
	if (Input::IsKeyDown(Input::GetCurrentInputmode(), EKey::KEY_F))
		RenderSystem::GetInstance().GetRenderGraph()->TriggerRenderStage("COMPUTE_HEALTH");

	// Update health buffer from GPU
	// Only try to copy if the signal has been signaled
	// if (m_CopyFence->GetValue() == m_FenceCounter)
	// {
	// 	// Validation layer has a bug in which it will not detect that the command list is done
	// 	// after a GetValue check, therefore this wait is added to suppress that.
	// 	// NOTE: It should not wait at all really, it is just to make sure it is _actually_ done.
	// 	m_CopyFence->Wait(m_FenceCounter, UINT64_MAX);
	// 	m_CommandAllocator->Reset();
	// 	m_CommandList->Begin(nullptr);
	// 	m_CommandList->CopyBuffer(m_HealthBuffer, 0, m_CopyBuffer, 0, sizeof(uint32) * 10);
	// 	m_CommandList->End();
	// 	RenderAPI::GetComputeQueue()->ExecuteCommandLists(
	// 		&m_CommandList,
	// 		1,
	// 		FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN,
	// 		nullptr,
	// 		0,
	// 		m_CopyFence,
	// 		++m_FenceCounter
	// 	);
	// }

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredHitInfoLock);
		if (!m_DeferredHitInfo.IsEmpty())
		{
			m_HitInfoToProcess = m_DeferredHitInfo;
			m_DeferredHitInfo.Clear();
		}
	}

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
		if (!m_DeferredResets.IsEmpty())
		{
			m_ResetsToProcess = m_DeferredResets;
			m_DeferredResets.Clear();
		}
	}

	// uint32* data = reinterpret_cast<uint32*>(m_CopyBuffer->Map());
	// memcpy(m_PlayerHealths.GetData(), data, sizeof(uint32) * 10);
	// m_CopyBuffer->Unmap();

	// TEMP REMOVE
	if (Input::IsKeyDown(Input::GetCurrentInputmode(), EKey::KEY_G))
	{
		for (auto health : m_PlayerHealths)
		{
			LOG_WARNING("Health: %d", health);
		}
	}


	// Update health
	// if (!m_HitInfoToProcess.IsEmpty())
	// {
	// 	// Transfer health data
	// 	uint32* data = reinterpret_cast<uint32*>(m_CopyBuffer->Map());
	// 	memcpy(m_PlayerHealths, data, sizeof(uint32) * 10);
	// 	m_CopyBuffer->Unmap();

	// 	ECSCore* pECS = ECSCore::GetInstance();
	// 	ComponentArray<HealthComponent>*		pHealthComponents		= pECS->GetComponentArray<HealthComponent>();
	// 	ComponentArray<MeshPaintComponent>*		pMeshPaintComponents	= pECS->GetComponentArray<MeshPaintComponent>();
	// 	ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	// 	for (HitInfo& hitInfo : m_HitInfoToProcess)
	// 	{
	// 		const Entity entity				= hitInfo.Player;
	// 		const Entity projectileOwner	= hitInfo.ProjectileOwner;

	// 		HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
	// 		PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);
	// 		MeshPaintComponent& meshPaintComponent			= pMeshPaintComponents->GetData(entity);

	// 		constexpr float32 BIASED_MAX_HEALTH	= 0.15f;
	// 		constexpr float32 START_HEALTH_F	= float32(START_HEALTH);

	// 		// Update health
	// 		const float32	paintedHealth	= float32(paintedPixels) / float32(m_VertexCount);
	// 		const int32		oldHealth		= healthComponent.CurrentHealth;
	// 		healthComponent.CurrentHealth	= std::max<int32>(int32(START_HEALTH_F * (1.0f - paintedHealth)), 0);

	// 		LOG_INFO("HIT REGISTERED: oldHealth=%d currentHealth=%d", oldHealth, healthComponent.CurrentHealth);

	// 		// Check if health changed
	// 		if (oldHealth != healthComponent.CurrentHealth)
	// 		{
	// 			LOG_INFO("PLAYER HEALTH: CurrentHealth=%u, paintedHealth=%.4f paintedPixels=%u MAX_PIXELS=%.4f",
	// 				healthComponent.CurrentHealth,
	// 				paintedHealth,
	// 				paintedPixels,
	// 				MAX_PIXELS);

	// 			bool killed = false;
	// 			if (healthComponent.CurrentHealth <= 0)
	// 			{
	// 				MatchServer::KillPlayer(entity, projectileOwner);
	// 				killed = true;

	// 				LOG_INFO("PLAYER DIED");
	// 			}

	// 			PacketHealthChanged packet = {};
	// 			packet.CurrentHealth = healthComponent.CurrentHealth;
	// 			packets.SendPacket(packet);
	// 		}
	// 	}

	// 	m_HitInfoToProcess.Clear();
	// }

	// // Reset healthcomponents
	// if (!m_ResetsToProcess.IsEmpty())
	// {
	// 	ECSCore* pECS = ECSCore::GetInstance();
	// 	ComponentArray<HealthComponent>*						pHealthComponents			= pECS->GetComponentArray<HealthComponent>();
	// 	ComponentArray<PacketComponent<PacketHealthChanged>>*	pHealthChangedComponents	= pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	// 	for (Entity entity : m_ResetsToProcess)
	// 	{
	// 		// Reset texture
	// 		MeshPaintHandler::ResetServer(entity);

	// 		// Reset health and send to client
	// 		HealthComponent& healthComponent	= pHealthComponents->GetData(entity);
	// 		healthComponent.CurrentHealth		= START_HEALTH;

	// 		PacketComponent<PacketHealthChanged>& packets = pHealthChangedComponents->GetData(entity);
	// 		PacketHealthChanged packet = {};
	// 		packet.CurrentHealth = healthComponent.CurrentHealth;
	// 		packets.SendPacket(packet);
	// 	}

	// 	m_ResetsToProcess.Clear();
	// }
}

bool HealthSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		HealthSystem::CreateBaseSystemRegistration(systemReg);
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
		{
			.pSubscriber = &m_MeshPaintEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() },
			},
		});

		systemReg.SubscriberRegistration.AdditionalAccesses.PushBack(
			{ R, ProjectileComponent::Type() }
		);
		
		RegisterSystem(TYPE_NAME(HealthSystemServer), systemReg);
	}

	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystemServer::OnProjectileHit);

	if (!CreateResources())
	{
		return false;
	}

	return true;
}

bool HealthSystemServer::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredHitInfoLock);
	for (Entity entity : m_HealthEntities)
	{
		// CollisionInfo0 is the projectile
		if (projectileHitEvent.CollisionInfo1.Entity == entity)
		{
			const Entity projectileEntity = projectileHitEvent.CollisionInfo0.Entity;
			
			ECSCore* pECS = ECSCore::GetInstance();
			const ProjectileComponent& projectileComponent = pECS->GetConstComponent<ProjectileComponent>(projectileEntity);
			
			m_DeferredHitInfo.PushBack(
				{ 
					projectileHitEvent.CollisionInfo1.Entity,
					projectileComponent.Owner
				});

			break;
		}
	}

	return true;
}

void HealthSystemServer::InternalResetHealth(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
	m_DeferredResets.EmplaceBack(entity);
}

void HealthSystemServer::ResetHealth(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	HealthSystemServer* pServerHealthSystem = static_cast<HealthSystemServer*>(s_Instance.Get());
	VALIDATE(pServerHealthSystem != nullptr);

	pServerHealthSystem->InternalResetHealth(entity);
}

bool HealthSystemServer::CreateResources()
{
	using namespace LambdaEngine;

	// Prepare health buffer
	// m_CommandAllocator	= RenderAPI::GetDevice()->CreateCommandAllocator("Health System Server Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

	// CommandListDesc commandListDesc	= {};
	// commandListDesc.DebugName		= "Health System Server Command List";
	// commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
	// commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
	// m_CommandList					= RenderAPI::GetDevice()->CreateCommandList(m_CommandAllocator, &commandListDesc);

	// BufferDesc healthBufferDesc		= {};
	// healthBufferDesc.DebugName		= "Health System Server Health Buffer";
	// healthBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
	// healthBufferDesc.SizeInBytes	= sizeof(uint32) * 10;
	// healthBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_SRC;
	// m_HealthBuffer					= RenderAPI::GetDevice()->CreateBuffer(&healthBufferDesc);

	// BufferDesc copyBufferDesc		= {};
	// copyBufferDesc.DebugName		= "Health System Server Health Buffer";
	// copyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
	// copyBufferDesc.SizeInBytes		= sizeof(uint32) * 10;
	// copyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
	// m_CopyBuffer					= RenderAPI::GetDevice()->CreateBuffer(&copyBufferDesc);

	// BufferDesc countBufferDesc		= {};
	// countBufferDesc.DebugName		= "Health System Server Vertices Count Buffer";
	// countBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
	// countBufferDesc.SizeInBytes		= sizeof(uint32);
	// countBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
	// m_VertexCountBuffer				= RenderAPI::GetDevice()->CreateBuffer(&countBufferDesc);

	// ResourceUpdateDesc updateHealthDesc = {};
	// updateHealthDesc.ResourceName					= "PLAYER_HEALTH_BUFFER";
	// updateHealthDesc.ExternalBufferUpdate.ppBuffer	= &m_HealthBuffer;
	// updateHealthDesc.ExternalBufferUpdate.Count		= 1;
	// RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&updateHealthDesc);

	// ResourceUpdateDesc updateCountDesc = {};
	// updateCountDesc.ResourceName					= "VERTEX_COUNT_BUFFER";
	// updateCountDesc.ExternalBufferUpdate.ppBuffer	= &m_VertexCountBuffer;
	// updateCountDesc.ExternalBufferUpdate.Count		= 1;
	// RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&updateCountDesc);

	// FenceDesc fenceDesc		= {};
	// fenceDesc.DebugName		= "Health System Fence";
	// fenceDesc.InitalValue	= m_FenceCounter;
	// m_CopyFence				= RenderAPI::GetDevice()->CreateFence(&fenceDesc);

	// // Transfer vertex count to buffer
	// {
	// 	BufferDesc stagingBufferDesc	= {};
	// 	stagingBufferDesc.DebugName		= "Health System Server Vertices Count Staging Buffer";
	// 	stagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
	// 	stagingBufferDesc.SizeInBytes	= sizeof(uint32);
	// 	stagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_SRC;
	// 	Buffer* stagingBuffer			= RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);

	// 	// Get mesh to know how many vertices there are
	// 	// NOTE: The mesh should already be loaded and should therefore
	// 	//		 not create it again.
	// 	GUID_Lambda player = 0;
	// 	TArray<GUID_Lambda> temp;
	// 	ResourceManager::LoadMeshFromFile("Player/IdleRightUV.glb", player, temp);
	// 	Mesh* pMesh = ResourceManager::GetMesh(player);
	// 	m_VertexCount = pMesh->Vertices.GetSize();

	// 	uint32* data = reinterpret_cast<uint32*>(stagingBuffer->Map());
	// 	memcpy(data, &m_VertexCount, sizeof(uint32));
	// 	stagingBuffer->Unmap();

	// 	m_CommandAllocator->Reset();
	// 	m_CommandList->Begin(nullptr);
	// 	m_CommandList->CopyBuffer(stagingBuffer, 0, m_VertexCountBuffer, 0, sizeof(uint32));
	// 	m_CommandList->End();
	// 	RenderAPI::GetComputeQueue()->ExecuteCommandLists(
	// 		&m_CommandList,
	// 		1,
	// 		FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN,
	// 		nullptr,
	// 		0,
	// 		m_CopyFence,
	// 		++m_FenceCounter
	// 	);

	// 	m_CopyFence->Wait(m_FenceCounter, UINT64_MAX);

	// 	SAFERELEASE(stagingBuffer);
	// }

	return true;
}