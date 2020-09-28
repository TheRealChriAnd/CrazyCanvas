#include "Game/ECS/Systems/Networking/ClientSystem.h"

#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem() : 
		m_ControllableEntities(),
		m_pClient(nullptr),
		m_Buffer(),
		m_SimulationTick(0)
	{
		ClientDesc clientDesc = {};
		clientDesc.PoolSize = 1024;
		clientDesc.MaxRetries = 10;
		clientDesc.ResendRTTMultiplier = 3.0F;
		clientDesc.Handler = this;
		clientDesc.Protocol = EProtocol::UDP;
		clientDesc.PingInterval = Timestamp::Seconds(1);
		clientDesc.PingTimeout = Timestamp::Seconds(3);
		clientDesc.UsePingSystem = true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);


		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{{{R, ControllableComponent::Type()}, {R, PositionComponent::Type()} }, {}, &m_ControllableEntities}
		};
		systemReg.Phase = 0;

		RegisterSystem(systemReg);
	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
	}

	bool ClientSystem::Connect(IPAddress* address)
	{
		if (!m_pClient->Connect(IPEndPoint(address, 4444)))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		if (m_pClient->IsConnected())
		{
			int8 deltaForward = int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
			int8 deltaLeft = int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
			BinaryEncoder encoder(pPacket);
			encoder.WriteInt32(m_SimulationTick);
			encoder.WriteInt8(deltaForward);
			encoder.WriteInt8(deltaLeft);
			m_pClient->SendUnreliable(pPacket);


			

			ECSCore* pECS = ECSCore::GetInstance();
			auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

			if (!pPositionComponents)
				return;

			GameState gameState = {};
			gameState.SimulationTick = m_SimulationTick;

			for (Entity entity : m_ControllableEntities)
			{
				PositionComponent& positionComponent = pPositionComponents->GetData(entity);

				if (deltaForward != 0)
				{
					gameState.Position.x = positionComponent.Position.x + 1.0f * Timestamp::Seconds(1.0f / 60.0f).AsSeconds() * deltaForward;
				}

				if (deltaLeft != 0)
				{
					gameState.Position.z = positionComponent.Position.z + 1.0f * Timestamp::Seconds(1.0f / 60.0f).AsSeconds() * deltaLeft;
				}
				break;
			}

			m_Buffer.Write(gameState);
			m_SimulationTick++;
		}
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		NetworkDebugger::RenderStatistics(m_pClient);
	}

	void ClientSystem::Tick(Timestamp deltaTime)
	{
		
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{

	}

	void ClientSystem::OnConnected(IClient* pClient)
	{

	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{

	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{

	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_ENTITY_CREATE)
		{
			BinaryDecoder decoder(pPacket);
			bool isMySelf		= decoder.ReadBool();
			int32 networkUID	= decoder.ReadInt32();
			glm::vec3 position	= decoder.ReadVec3();
			glm::vec3 color		= decoder.ReadVec3();

			if (isMySelf)
				m_NetworkUID = networkUID;

			Job addEntityJob;
			addEntityJob.Components =
			{
				{ RW, PositionComponent::Type()		},
				{ RW, RotationComponent::Type()		},
				{ RW, ScaleComponent::Type()		},
				{ RW, MeshComponent::Type()			},
				{ RW, NetworkComponent::Type()		},
				{ RW, ControllableComponent::Type() }
			};
			addEntityJob.Function = std::bind(&ClientSystem::CreateEntity, this, networkUID, position, color);

			ECSCore::GetInstance()->ScheduleJobASAP(addEntityJob);
		}
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{

	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{

	}

	void ClientSystem::CreateEntity(int32 networkUID, const glm::vec3& position, const glm::vec3& color)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		Entity entity = pECS->CreateEntity();

		MaterialProperties materialProperties = {};
		materialProperties.Roughness = 0.1f;
		materialProperties.Metallic = 0.0f;
		materialProperties.Albedo = glm::vec4(color, 1.0f);

		MeshComponent meshComponent;
		meshComponent.MeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Mirror Material" + std::to_string(entity),
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		pECS->AddComponent<PositionComponent>(entity,		{ position, true });
		pECS->AddComponent<RotationComponent>(entity,		{ glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity,			{ glm::vec3(1.0f), true });
		pECS->AddComponent<MeshComponent>(entity,			meshComponent);
		pECS->AddComponent<NetworkComponent>(entity,		{ networkUID });

		if(m_NetworkUID == networkUID)
			pECS->AddComponent<ControllableComponent>(entity,	{ true });
	}

	void ClientSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
	}

	void ClientSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ClientSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}