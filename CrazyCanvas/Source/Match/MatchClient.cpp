#include "Match/MatchClient.h"
#include "Match/Match.h"

#include "Multiplayer/Packet/PacketType.h"

#include "World/LevelObjectCreator.h"
#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/CommonApplication.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

using namespace LambdaEngine;

MatchClient::~MatchClient()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketCreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketDeleteLevelObject>>(this, &MatchClient::OnPacketDeleteLevelObjectReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameOver>>(this, &MatchClient::OnPacketGameOverReceived);
}

bool MatchClient::InitInternal()
{
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketCreateLevelObject>>(this, &MatchClient::OnPacketCreateLevelObjectReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &MatchClient::OnPacketTeamScoredReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketDeleteLevelObject>>(this, &MatchClient::OnPacketDeleteLevelObjectReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameOver>>(this, &MatchClient::OnPacketGameOverReceived);
	return true;
}

void MatchClient::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

bool MatchClient::OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<PacketCreateLevelObject>& event)
{
	const PacketCreateLevelObject& packet = event.Packet;

	switch (packet.LevelObjectType)
	{
		case ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER:
		{
			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

			const CameraDesc cameraDesc =
			{
				.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV"),
				.Width = (float)window->GetWidth(),
				.Height = (float)window->GetHeight(),
				.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane"),
				.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane")
			};

			CreatePlayerDesc createPlayerDesc =
			{
				.IsLocal			= packet.Player.IsMySelf,
				.PlayerNetworkUID	= packet.NetworkUID,
				.WeaponNetworkUID	= packet.Player.WeaponNetworkUID,
				.Position			= packet.Position,
				.Forward			= packet.Forward,
				.Scale				= glm::vec3(1.0f),
				.TeamIndex			= packet.Player.TeamIndex,
				.pCameraDesc		= &cameraDesc,
			};

			TArray<Entity> createdPlayerEntities;
			if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER, &createPlayerDesc, createdPlayerEntities))
			{
				LOG_ERROR("[MatchClient]: Failed to create Player!");
			}

			break;
		}
		case ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG:
		{
			Entity parentEntity = MultiplayerUtils::GetEntity(packet.Flag.ParentNetworkUID);

			CreateFlagDesc createFlagDesc =
			{
				.NetworkUID = packet.NetworkUID,
				.ParentEntity = parentEntity,
				.Position = packet.Position,
				.Scale = glm::vec3(1.0f),
				.Rotation = glm::quatLookAt(packet.Forward, g_DefaultUp),
			};

			TArray<Entity> createdFlagEntities;
			if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG, &createFlagDesc, createdFlagEntities))
			{
				LOG_ERROR("[MatchClient]: Failed to create Flag!");
			}

			break;
		}
	}
	return true;
}

bool MatchClient::OnPacketTeamScoredReceived(const PacketReceivedEvent<PacketTeamScored>& event)
{
	const PacketTeamScored& packet = event.Packet;
	SetScore(packet.TeamIndex, packet.Score);
	return true;
}

bool MatchClient::OnPacketDeleteLevelObjectReceived(const PacketReceivedEvent<PacketDeleteLevelObject>& event)
{
	const PacketDeleteLevelObject& packet = event.Packet;
	Entity entity = MultiplayerUtils::GetEntity(packet.NetworkUID);

	if(entity != UINT32_MAX)
		m_pLevel->DeleteObject(entity);

	return true;
}

bool MatchClient::OnPacketGameOverReceived(const PacketReceivedEvent<PacketGameOver>& event)
{
	const PacketGameOver& packet = event.Packet;

	LOG_INFO("Game Over, Winning team is %d", packet.WinningTeamIndex);
	ResetMatch();

	return true;
}

bool MatchClient::OnWeaponFired(const WeaponFiredEvent& event)
{
	using namespace LambdaEngine;

	CreateProjectileDesc createProjectileDesc;
	createProjectileDesc.AmmoType		= event.AmmoType;
	createProjectileDesc.FireDirection	= event.Direction;
	createProjectileDesc.FirePosition	= event.Position;
	createProjectileDesc.InitalVelocity = event.InitialVelocity;
	createProjectileDesc.TeamIndex		= event.TeamIndex;
	createProjectileDesc.Callback		= event.Callback;
	createProjectileDesc.MeshComponent	= event.MeshComponent;
	createProjectileDesc.WeaponOwner	= event.WeaponOwnerEntity;

	TArray<Entity> createdFlagEntities;
	if (!m_pLevel->CreateObject(ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE, &createProjectileDesc, createdFlagEntities))
	{
		LOG_ERROR("[MatchClient]: Failed to create projectile!");
	}

	LOG_INFO("CLIENT: Weapon fired");
	return true;
}