#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/Systems/Player/HealthSystemClient.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Events/GameplayEvents.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include <mutex>

/*
* HealthSystem
*/

HealthSystem* HealthSystem::s_pInstance = nullptr;

bool HealthSystem::Init()
{
	using namespace LambdaEngine;

	if (MultiplayerUtils::IsServer())
	{
		s_pInstance = DBG_NEW HealthSystemServer();
	}
	else
	{
		s_pInstance = DBG_NEW HealthSystemClient();
	}

	return s_pInstance->InitInternal();
}

void HealthSystem::Release()
{
	SAFEDELETE(s_pInstance);
}

bool HealthSystem::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= NDA;
		playerGroup.Rotation.Permissions	= NDA;
		playerGroup.Velocity.Permissions	= NDA;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_HealthEntities,
				.ComponentAccesses =
				{
					{ RW, HealthComponent::Type() },
					{ RW, PacketComponent<PacketHealthChanged>::Type() }
				}
			},
			{
				.pSubscriber = &m_LocalPlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerLocalComponent::Type() },
				},
				.ComponentGroups = { &playerGroup }
			},
		};

		// After weaponsystem
		systemReg.Phase = 2;

		RegisterSystem(TYPE_NAME(HealthSystem), systemReg);
	}

	return true;
}
