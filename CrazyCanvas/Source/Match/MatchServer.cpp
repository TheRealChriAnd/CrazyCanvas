#include "Match/MatchServer.h"
#include "Match/Match.h"

#include "ECS/ECSCore.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

#include "Math/Random.h"

#include "Networking/API/ClientRemoteBase.h"

#include "Rendering/ImGuiRenderer.h"

#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketMatchReady.h"
#include "Multiplayer/Packet/PacketMatchStart.h"
#include "Multiplayer/Packet/PacketMatchBegin.h"

#include "Lobby/PlayerManagerServer.h"

#include <imgui.h>

#define RENDER_MATCH_INFORMATION

MatchServer::~MatchServer()
{
	using namespace LambdaEngine;
	
	EventQueue::UnregisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);
}

void MatchServer::KillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity)
{
	using namespace LambdaEngine;

	const Player* pPlayer = PlayerManagerServer::GetPlayer(entityToKill);
	const Player* pPlayerKiller = PlayerManagerServer::GetPlayer(killedByEntity);
	PlayerManagerServer::SetPlayerAlive(pPlayer, false, pPlayerKiller);

	std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
	m_PlayersToKill.EmplaceBack(entityToKill);

	std::scoped_lock<SpinLock> lock(m_PlayersToRespawnLock);
	m_PlayersToRespawn.EmplaceBack(std::make_pair(entityToKill, 5.0f));
}

bool MatchServer::InitInternal()
{
	using namespace LambdaEngine;

	EventQueue::RegisterEventHandler<FlagDeliveredEvent>(this, &MatchServer::OnFlagDelivered);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &MatchServer::OnClientDisconnected);

	return true;
}

void MatchServer::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (m_ShouldBeginMatch && !m_HasBegun)
	{
		m_MatchBeginTimer -= float32(deltaTime.AsSeconds());

		if (m_MatchBeginTimer < 0.0f)
		{
			MatchBegin();
		}
	}

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (flagEntities.IsEmpty())
			SpawnFlag();
	}

	//Render Some Server Match Information
#if defined(RENDER_MATCH_INFORMATION)
	ImGuiRenderer::Get().DrawUI([this]()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		if (ImGui::Begin("Match Panel"))
		{
			if (m_pLevel != nullptr)
			{
				// Flags
				TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
				if (!flagEntities.IsEmpty())
				{
					Entity flagEntity = flagEntities[0];

					std::string name = "Flag : [EntityID=" + std::to_string(flagEntity) + "]";
					if (ImGui::TreeNode(name.c_str()))
					{
						const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
						const PositionComponent& flagPositionComponent = pECS->GetConstComponent<PositionComponent>(flagEntity);
						ImGui::Text("Flag Position: [ %f, %f, %f ]", flagPositionComponent.Position.x, flagPositionComponent.Position.y, flagPositionComponent.Position.z);
						ImGui::Text("Flag Status: %s", flagParentComponent.Attached ? "Carried" : "Not Carried");

						if (flagParentComponent.Attached)
						{
							if (ImGui::Button("Drop Flag"))
							{
								TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

								if (!flagSpawnPointEntities.IsEmpty())
								{
									Entity flagSpawnPoint = flagSpawnPointEntities[0];
									const FlagSpawnComponent& flagSpawnComponent	= pECS->GetConstComponent<FlagSpawnComponent>(flagSpawnPoint);
									const PositionComponent& positionComponent		= pECS->GetConstComponent<PositionComponent>(flagSpawnPoint);

									float r		= Random::Float32(0.0f, flagSpawnComponent.Radius);
									float theta = Random::Float32(0.0f, glm::two_pi<float32>());
									glm::vec3 flagPosition = positionComponent.Position + r * glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta));

									FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, flagPosition);
								}
							}
						}

						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::Text("Flag Status: Not Spawned");
				}

				// Player
				TArray<Entity> playerEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);
				if (!playerEntities.IsEmpty())
				{
					ComponentArray<ChildComponent>*		pChildComponents	= pECS->GetComponentArray<ChildComponent>();
					ComponentArray<HealthComponent>*	pHealthComponents	= pECS->GetComponentArray<HealthComponent>();
					ComponentArray<WeaponComponent>*	pWeaponComponents	= pECS->GetComponentArray<WeaponComponent>();

					ImGui::Text("Player Status:");
					for (Entity playerEntity : playerEntities)
					{
						const Player* pPlayer = PlayerManagerServer::GetPlayer(playerEntity);
						if (!pPlayer)
							continue;

						std::string name = pPlayer->GetName() + ": [EntityID=" + std::to_string(playerEntity) + "]";
						if (ImGui::TreeNode(name.c_str()))
						{
							const HealthComponent& health = pHealthComponents->GetConstData(playerEntity);
							ImGui::Text("Health: %u", health.CurrentHealth);

							const ChildComponent& children = pChildComponents->GetConstData(playerEntity);
							Entity weapon = children.GetEntityWithTag("weapon");

							const WeaponComponent& weaponComp = pWeaponComponents->GetConstData(weapon);

							auto waterAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
							if(waterAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Water Ammunition: %u/%u", waterAmmo->second.first, waterAmmo->second.second);

							auto paintAmmo = weaponComp.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
							if (paintAmmo != weaponComp.WeaponTypeAmmo.end())
								ImGui::Text("Paint Ammunition: %u/%u", paintAmmo->second.first, paintAmmo->second.second);


							if (ImGui::Button("Kill"))
							{
								Match::KillPlayer(playerEntity, UINT32_MAX);
							}
							
							ImGui::SameLine();

							if (ImGui::Button("Disconnect"))
							{
								ServerHelper::DisconnectPlayer(pPlayer, "Kicked");
							}

							ImGui::TreePop();
						}
					}
				}
				else
				{
					ImGui::Text("Player Status: No players");
				}
			}
		}

		ImGui::End();
	});
#endif
}

void MatchServer::BeginLoading()
{
	using namespace LambdaEngine;
	LOG_INFO("SERVER: Loading started!");

	const THashTable<uint64, Player>& players = PlayerManagerBase::GetPlayers();

	for (auto& pair : players)
	{
		SpawnPlayer(pair.second);
	}

	// Send flag data to clients
	{
		ECSCore* pECS = ECSCore::GetInstance();

		ComponentArray<PositionComponent>* pPositionComponents	= pECS->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>* pRotationComponents	= pECS->GetComponentArray<RotationComponent>();
		ComponentArray<ParentComponent>* pParentComponents		= pECS->GetComponentArray<ParentComponent>();

		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		PacketCreateLevelObject packet;
		packet.LevelObjectType = ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;

		for (Entity flagEntity : flagEntities)
		{
			const PositionComponent& positionComponent	= pPositionComponents->GetConstData(flagEntity);
			const RotationComponent& rotationComponent	= pRotationComponents->GetConstData(flagEntity);
			const ParentComponent& parentComponent		= pParentComponents->GetConstData(flagEntity);

			packet.NetworkUID				= flagEntity;
			packet.Position					= positionComponent.Position;
			packet.Forward					= GetForward(rotationComponent.Quaternion);
			packet.Flag.ParentNetworkUID	= parentComponent.Parent;
			ServerHelper::SendBroadcast(packet);
		}
	}

	PacketMatchReady packet;
	ServerHelper::SendBroadcast(packet);
}

void MatchServer::MatchStart()
{
	m_HasBegun = false;
	m_ShouldBeginMatch = true;
	m_MatchBeginTimer = MATCH_BEGIN_COUNTDOWN_TIME;

	PacketMatchStart matchStartPacket;
	ServerHelper::SendBroadcast(matchStartPacket);

	LOG_INFO("SERVER: Match Start");
}

void MatchServer::MatchBegin()
{
	m_HasBegun = true;
	m_ShouldBeginMatch = false;

	PacketMatchBegin matchBeginPacket;
	ServerHelper::SendBroadcast(matchBeginPacket);

	LOG_INFO("SERVER: Match Begin");
}

void MatchServer::SpawnPlayer(const Player& player)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> playerSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	glm::vec3 position(0.0f, 5.0f, 0.0f);
	glm::vec3 forward(0.0f, 0.0f, 1.0f);

	for (Entity spawnPoint : playerSpawnPointEntities)
	{
		const TeamComponent& teamComponent = pTeamComponents->GetConstData(spawnPoint);

		if (teamComponent.TeamIndex == m_NextTeamIndex)
		{
			const PositionComponent& positionComponent = pPositionComponents->GetConstData(spawnPoint);
			position = positionComponent.Position + glm::vec3(0.0f, 1.0f, 0.0f);
			forward = glm::normalize(-glm::vec3(position.x, 0.0f, position.z));
			break;
		}
	}

	CreatePlayerDesc createPlayerDesc =
	{
		.ClientUID	= player.GetUID(),
		.Position	= position,
		.Forward	= forward,
		.Scale		= glm::vec3(1.0f),
		.TeamIndex	= m_NextTeamIndex,
	};

	TArray<Entity> createdPlayerEntities;
	if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
	{
		VALIDATE(createdPlayerEntities.GetSize() == 1);

		PacketCreateLevelObject packet;
		packet.LevelObjectType	= ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER;
		packet.Position			= position;
		packet.Forward			= forward;
		packet.Player.TeamIndex = m_NextTeamIndex;

		ComponentArray<ChildComponent>* pCreatedChildComponents = pECS->GetComponentArray<ChildComponent>();
		for (Entity playerEntity : createdPlayerEntities)
		{
			const ChildComponent& childComp = pCreatedChildComponents->GetConstData(playerEntity);
			packet.Player.ClientUID			= player.GetUID();
			packet.NetworkUID				= playerEntity;
			packet.Player.WeaponNetworkUID	= childComp.GetEntityWithTag("weapon");
			ServerHelper::SendBroadcast(packet);
		}
	}
	else
	{
		LOG_ERROR("[MatchServer]: Failed to create Player");
	}

	m_NextTeamIndex = (m_NextTeamIndex + 1) % 2;
}

void MatchServer::FixedTickInternal(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(deltaTime);

	{
		std::scoped_lock<SpinLock> lock(m_PlayersToKillLock);
		for (Entity playerEntity : m_PlayersToKill)
		{
			LOG_INFO("SERVER: Player=%u DIED", playerEntity);
			KillPlayerInternal(playerEntity);
		}

		for (PlayerTimers player : m_PlayersToRespawn)
		{
			player.second -= float32(deltaTime.AsSeconds());

			if (player.second <= 0.0f)
			{
				RespawnPlayer(player.first);
				LOG_INFO("SERVER: Player=%u RESPAWNED", player.first);
			}
		}

		m_PlayersToKill.Clear();
	}
}

void MatchServer::SpawnFlag()
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

	if (!flagSpawnPointEntities.IsEmpty())
	{
		Entity flagSpawnPoint = flagSpawnPointEntities[0];
		const FlagSpawnComponent& flagSpawnComponent	= pECS->GetConstComponent<FlagSpawnComponent>(flagSpawnPoint);
		const PositionComponent& positionComponent		= pECS->GetConstComponent<PositionComponent>(flagSpawnPoint);

		float r		= Random::Float32(0.0f, flagSpawnComponent.Radius);
		float theta = Random::Float32(0.0f, glm::two_pi<float32>());

		CreateFlagDesc createDesc = {};
		createDesc.Position		= positionComponent.Position + r * glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta));
		createDesc.Scale		= glm::vec3(1.0f);
		createDesc.Rotation		= glm::identity<glm::quat>();

		TArray<Entity> createdFlagEntities;
		if (m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createDesc, createdFlagEntities))
		{
			VALIDATE(createdFlagEntities.GetSize() == 1);

			PacketCreateLevelObject packet;
			packet.LevelObjectType			= ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG;
			packet.Position					= createDesc.Position;
			packet.Forward					= GetForward(createDesc.Rotation);
			packet.Flag.ParentNetworkUID	= INT32_MAX;

			//Tell the bois that we created a flag
			for (Entity entity : createdFlagEntities)
			{
				packet.NetworkUID = entity;

				ServerHelper::SendBroadcast(packet);
			}
		}
		else
		{
			LOG_ERROR("[MatchServer]: Failed to create Flag");
		}
	}
}

bool MatchServer::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity	= event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;
	createProjectileDesc.MeshComponent	= event.MeshComponent;
	createProjectileDesc.Angle			= event.Angle;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchServer]: Failed to create projectile!");
	}

	LOG_INFO("SERVER: Weapon fired");
	return false;
}

void MatchServer::DeleteGameLevelObject(LambdaEngine::Entity entity)
{
	m_pLevel->DeleteObject(entity);

	PacketDeleteLevelObject packet;
	packet.NetworkUID = entity;

	ServerHelper::SendBroadcast(packet);
}

bool MatchServer::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	VALIDATE(event.pClient != nullptr);

	const Player* pPlayer = PlayerManagerBase::GetPlayer(event.pClient);
	if (pPlayer)
	{
		Match::KillPlayer(pPlayer->GetEntity(), UINT32_MAX);
	}
	

	// TODO: Fix this
	//DeleteGameLevelObject(playerEntity);

	return true;
}

bool MatchServer::OnFlagDelivered(const FlagDeliveredEvent& event)
{
	using namespace LambdaEngine;

	if (m_pLevel != nullptr)
	{
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);

		if (!flagEntities.IsEmpty())
		{
			Entity flagEntity = flagEntities[0];

			TArray<Entity> flagSpawnPointEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN);

			if (!flagSpawnPointEntities.IsEmpty())
			{
				ECSCore* pECS = ECSCore::GetInstance();

				Entity flagSpawnPoint = flagSpawnPointEntities[0];
				const FlagSpawnComponent& flagSpawnComponent = pECS->GetConstComponent<FlagSpawnComponent>(flagSpawnPoint);
				const PositionComponent& positionComponent = pECS->GetConstComponent<PositionComponent>(flagSpawnPoint);

				float r = Random::Float32(0.0f, flagSpawnComponent.Radius);
				float theta = Random::Float32(0.0f, glm::two_pi<float32>());
				glm::vec3 flagPosition = positionComponent.Position + r * glm::vec3(glm::cos(theta), 0.0f, glm::sin(theta));

				FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, flagPosition);
			}
		}
	}

	uint32 newScore = GetScore(event.TeamIndex) + 1;
	SetScore(event.TeamIndex, newScore);

	PacketTeamScored packet;
	packet.TeamIndex	= event.TeamIndex;
	packet.Score		= newScore;
	ServerHelper::SendBroadcast(packet);

	if (newScore == m_MatchDesc.MaxScore) // game over
	{
		PacketGameOver gameOverPacket;
		gameOverPacket.WinningTeamIndex = event.TeamIndex;

		ServerHelper::SendBroadcast(gameOverPacket);

		ResetMatch();
	}


	return true;
}

void MatchServer::KillPlayerInternal(LambdaEngine::Entity playerEntity)
{
	using namespace LambdaEngine;

	// MUST HAPPEN ON MAIN THREAD IN FIXED TICK FOR NOW
	ECSCore* pECS = ECSCore::GetInstance();
	NetworkPositionComponent& positionComp = pECS->GetComponent<NetworkPositionComponent>(playerEntity);

	// Get spawnpoint from level
	const glm::vec3 oldPosition = positionComp.Position;
	glm::vec3 jailPosition = glm::vec3(0.0f);
	//glm::vec3 spawnPosition = glm::vec3(0.0f);
	if (m_pLevel != nullptr)
	{
		// Retrive spawnpoints
		//TArray<Entity> spawnPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);
		TArray<Entity> jailPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_JAIL);

		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		/*const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

		uint8 playerTeam = pTeamComponents->GetConstData(playerEntity).TeamIndex;
		for (Entity spawnEntity : spawnPoints)
		{
			if (pTeamComponents->HasComponent(spawnEntity))
			{
				if (pTeamComponents->GetConstData(spawnEntity).TeamIndex == playerTeam)
				{
					spawnPosition = pPositionComponents->GetConstData(spawnEntity).Position;
				}
			}
		}*/

		for (Entity jailEntity : jailPoints)
		{
			jailPosition = pPositionComponents->GetConstData(jailEntity).Position;
		}

		// Drop flag if player carries it
		TArray<Entity> flagEntities = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG);
		if (!flagEntities.IsEmpty())
		{
			Entity flagEntity = flagEntities[0];

			const ParentComponent& flagParentComponent = pECS->GetConstComponent<ParentComponent>(flagEntity);
			if (flagParentComponent.Attached && flagParentComponent.Parent == playerEntity)
			{
				FlagSystemBase::GetInstance()->OnFlagDropped(flagEntity, oldPosition);
			}
		}
	}


	/*
	// Jail position
	positionComp.Position = jailPosition;

	// Reset Health
	HealthSystem::GetInstance().ResetEntityHealth(playerEntity);

	//Set player alive again
	const Player* pPlayer = PlayerManagerServer::GetPlayer(playerEntity);
	PlayerManagerServer::SetPlayerAlive(pPlayer, true, nullptr);*/
}

void MatchServer::RespawnPlayer(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	NetworkPositionComponent& positionComp = pECS->GetComponent<NetworkPositionComponent>(playerEntity);

	glm::vec3 spawnPosition = glm::vec3(0.0f);

	if (m_pLevel != nullptr)
	{
		TArray<Entity> spawnPoints = m_pLevel->GetEntities(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN);

		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

		uint8 playerTeam = pTeamComponents->GetConstData(playerEntity).TeamIndex;
		for (Entity spawnEntity : spawnPoints)
		{
			if (pTeamComponents->HasComponent(spawnEntity))
			{
				if (pTeamComponents->GetConstData(spawnEntity).TeamIndex == playerTeam)
				{
					spawnPosition = pPositionComponents->GetConstData(spawnEntity).Position;
				}
			}
		}
	}

	// Spawn position
	positionComp.Position = spawnPosition;

	// Reset Health
	HealthSystem::GetInstance().ResetEntityHealth(playerEntity);

	//Set player alive again
	const Player* pPlayer = PlayerManagerServer::GetPlayer(playerEntity);
	PlayerManagerServer::SetPlayerAlive(pPlayer, true, nullptr);

}
