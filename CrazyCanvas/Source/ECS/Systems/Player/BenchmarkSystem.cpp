#include "ECS/Systems/Player/BenchmarkSystem.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Input/API/Input.h"

#include "Physics/PhysicsEvents.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

void BenchmarkSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		// The write permissions are used when creating projectile entities
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions = RW;
		playerGroup.Scale.Permissions = RW;
		playerGroup.Rotation.Permissions = RW;
		playerGroup.Velocity.Permissions = RW;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_WeaponEntities,
				.ComponentAccesses =
				{
					{R, WeaponComponent::Type()}
				}
			},
			{
				.pSubscriber = &m_PlayerEntities,
				.ComponentAccesses =
				{
					{NDA, PlayerForeignComponent::Type()}
				},
				.ComponentGroups = { &playerGroup }
			}
		};
		systemReg.Phase = 0u;

		RegisterSystem(systemReg);
	}
}

void BenchmarkSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

	/*	Create jobs that fire weapons. This is a bit ugly since this requires BenchmarkSystem to know what components
		are accessed when WeaponSystem fires projectiles. */
	TArray<Entity> weaponEntitiesToFire;
	weaponEntitiesToFire.Reserve(m_WeaponEntities.Size());

	for (Entity weaponEntity : m_WeaponEntities)
	{
		const WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		const Entity playerEntity = weaponComponent.WeaponOwner;
		if (!m_PlayerEntities.HasElement(playerEntity))
		{
			continue;
		}

		// Rotate player
		constexpr const float32 rotationSpeed = 3.14f / 4.0f;
		RotationComponent& rotationComp = pRotationComponents->GetData(playerEntity);
		rotationComp.Quaternion = glm::rotate(rotationComp.Quaternion, rotationSpeed * dt, g_DefaultUp);

		weaponEntitiesToFire.PushBack(weaponEntity);
	}

	// Fire weapons
	const Job fireJob =
	{
		.Function = [weaponEntitiesToFire]
		{
			WeaponSystem* pWeaponSystem = WeaponSystem::GetInstance();
			ECSCore* pECS = ECSCore::GetInstance();

			const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
			const ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
			const ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
			ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();

			Entity weaponEntity = weaponEntitiesToFire[0];
			// for (Entity weaponEntity : weaponEntitiesToFire)
			// {
				WeaponComponent& weaponComp = pWeaponComponents->GetData(weaponEntity);
				const PositionComponent& playerPositionComp = pPositionComponents->GetConstData(weaponComp.WeaponOwner);
				const RotationComponent& playerRotationComp = pRotationComponents->GetConstData(weaponComp.WeaponOwner);
				const VelocityComponent& playerVelocityComp = pVelocityComponents->GetConstData(weaponComp.WeaponOwner);
				pWeaponSystem->TryFire(EAmmoType::AMMO_TYPE_PAINT, weaponComp, playerPositionComp.Position, playerRotationComp.Quaternion, playerVelocityComp.Velocity);
			// }
		},
		.Components = GetFireProjectileComponentAccesses(),
	};

	pECS->ScheduleJobASAP(fireJob);
}
