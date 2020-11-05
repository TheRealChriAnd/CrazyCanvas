#include "ECS/Systems/GUI/HUDSystem.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"


#include "Application/API/Events/EventQueue.h"
#include "..\..\..\..\Include\ECS\Systems\GUI\HUDSystem.h"


using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();

	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::UnregisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::UnregisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
}

void HUDSystem::Init()
{

	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_WeaponEntities,
			.ComponentAccesses =
			{
				{ R, WeaponComponent::Type() },
			}
		},
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{ R, HealthComponent::Type() }, { R, RotationComponent::Type() }, { NDA, PlayerLocalComponent::Type() }
			}
		}
	};
	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::RegisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
    EventQueue::RegisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);

	m_HUDGUI = *new HUDGUI();
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	GUIApplication::SetView(m_View);
}

void HUDSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
}

void HUDSystem::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();

	for (Entity players : m_PlayerEntities)
	{
		const HealthComponent& healthComponent = pHealthComponents->GetConstData(players);
		m_HUDGUI->UpdateScore();
		m_HUDGUI->UpdateHealth(healthComponent.CurrentHealth);
	}
}

bool HUDSystem::OnWeaponFired(const WeaponFiredEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		for (Entity playerWeapon : m_WeaponEntities)
		{
			const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

			if (pPlayerLocalComponents->HasComponent(weaponComponent.WeaponOwner) && m_HUDGUI)
			{
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, event.AmmoType);
			}
		}
	}
	return false;
}

bool HUDSystem::OnWeaponReloadFinished(const WeaponReloadFinishedEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		for (Entity playerWeapon : m_WeaponEntities)
		{
			const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

			if (event.WeaponOwnerEntity == weaponComponent.WeaponOwner && m_HUDGUI)
			{
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, EAmmoType::AMMO_TYPE_PAINT);
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, EAmmoType::AMMO_TYPE_WATER);
			}
		}
	}
	return false;
}

bool HUDSystem::OnMatchCountdownEvent(const MatchCountdownEvent& event)
{
	m_HUDGUI->UpdateCountdown(event.CountDownTime);

	return false;
}

bool HUDSystem::OnProjectileHit(const ProjectileHitEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();
		const ComponentArray<RotationComponent>* pPlayerRotationComp = pECS->GetComponentArray<RotationComponent>();
		if (pPlayerLocalComponents->HasComponent(event.CollisionInfo1.Entity))
		{
			LOG_INFO("Player on team %d hit a player", (int)event.Team);
			const RotationComponent& playerRotationComp = pPlayerRotationComp->GetConstData(event.CollisionInfo1.Entity);

			m_HUDGUI->DisplayHitIndicator(GetForward(glm::normalize(playerRotationComp.Quaternion)), event.CollisionInfo1.Normal);
		}
	}

	return false;
}
