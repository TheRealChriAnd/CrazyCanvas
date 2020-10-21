#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Application/API/Events/EventQueue.h"

#include <mutex>

HealthSystem::HealthSystem()
	: m_HealthEntities()
{
}

HealthSystem::~HealthSystem()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HealthSystem::OnProjectileHit);
}

bool HealthSystem::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DefferedEventsLock);
	LOG_INFO("Something got hit CollisionInfo0=%d, CollisionInfo1=%d", projectileHitEvent.CollisionInfo0.Entity, projectileHitEvent.CollisionInfo1.Entity);
	
	for (Entity entity : m_HealthEntities)
	{
		LOG_INFO("....Entity: %d", entity);

		// CollisionInfo0 is the projectile
		if (projectileHitEvent.CollisionInfo0.Entity == entity)
		{
			m_DeferredHitEvents.EmplaceBack(projectileHitEvent);
			LOG_INFO("Player got hit");
			break;
		}
	}

	return true;
}

bool HealthSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_HealthEntities,
				.ComponentAccesses =
				{
					{ RW, HealthComponent::Type() }
				}
			}
		};

		// After weaponsystem -> Do not know if this is correct
		systemReg.Phase = 2;

		RegisterSystem(systemReg);
	}

	// Register eventhandler
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystem::OnProjectileHit);

	return true;
}

void HealthSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pEcs = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* healthComponents = pEcs->GetComponentArray<HealthComponent>();

	// Since the events are not sent threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DefferedEventsLock);
		m_EventsToProcess = m_DeferredHitEvents;
		m_DeferredHitEvents.Clear();
	}

	for (ProjectileHitEvent& event : m_EventsToProcess)
	{
		// CollisionInfo1 is the entity that got hit
		Entity entity		= event.CollisionInfo1.Entity;
		EAmmoType ammoType	= event.AmmoType;
		
		HealthComponent& healthComponent = healthComponents->GetData(entity);
		// Hmm... better solution.. maybe?? 
		if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			healthComponent.CurrentHealth -= 10;
			LOG_INFO("Player damaged");
		}
		else if (ammoType == EAmmoType::AMMO_TYPE_WATER)
		{
			healthComponent.CurrentHealth += 10;
			LOG_INFO("Player got splashed");
		}
	}
}
