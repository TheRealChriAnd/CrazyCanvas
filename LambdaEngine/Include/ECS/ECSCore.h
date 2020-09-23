#pragma once

#include "ECS/ComponentStorage.h"
#include "ECS/ComponentType.h"
#include "ECS/EntityPublisher.h"
#include "ECS/EntityRegistry.h"
#include "ECS/JobScheduler.h"
#include "ECS/System.h"
#include "Utilities/IDGenerator.h"

#include <typeindex>

namespace LambdaEngine
{
	class EntitySubscriber;
	class RegularWorker;

    class LAMBDA_API ECSCore
    {
    public:
        ECSCore();
        ~ECSCore() = default;

		ECSCore(const ECSCore& other) = delete;
		void operator=(const ECSCore& other) = delete;

		static void Release();

        void Tick(Timestamp deltaTime);

		Entity CreateEntity() { return m_EntityRegistry.CreateEntity(); }

		// Add a component to a specific entity.
		template<typename Comp>
		Comp& AddComponent(Entity entity, const Comp& component);

		// Fetch a reference to a component for a specific entity.
		template<typename Comp>
		Comp& GetComponent(Entity entity);

		// Fetch a const reference to a component for a specific entity.
		template<typename Comp>
		const Comp& GetComponent(Entity entity) const;

		// Fetch a pointer to an array containing all components of a specific type.
		template<typename Comp>
		ComponentArray<Comp>* GetComponentArray();

		// Fetch a const pointer to an array containing all components of a specific type.
		template<typename Comp>
		const ComponentArray<Comp>* GetComponentArray() const;

		// RemoveComponent enqueues the removal of a component, which is performed at the end of the current/next frame.
		template<typename Comp>
		void RemoveComponent(Entity entity);

		// RemoveEntity enqueues the removal of an entity, which is performed at the end of the current/next frame.
		void RemoveEntity(Entity entity);

		void ScheduleJobASAP(const Job& job);
		void ScheduleJobPostFrame(const Job& job);

		void AddRegistryPage();
		void DeregisterTopRegistryPage();
		void DeleteTopRegistryPage();
		void ReinstateTopRegistryPage();

        Timestamp GetDeltaTime() const { return m_DeltaTime; }

	public:
		static ECSCore* GetInstance() { return s_pInstance; }

	protected:
		friend EntitySubscriber;
		uint32 SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration) { return m_EntityPublisher.SubscribeToEntities(subscriberRegistration); }
		void UnsubscribeFromEntities(uint32 subscriptionID) { m_EntityPublisher.UnsubscribeFromEntities(subscriptionID); }

		friend RegularWorker;
		uint32 ScheduleRegularJob(const Job& job, uint32_t phase)   { return m_JobScheduler.ScheduleRegularJob(job, phase); };
		void DescheduleRegularJob(uint32_t phase, uint32 jobID)     { m_JobScheduler.DescheduleRegularJob(phase, jobID); };

	private:
		void PerformComponentRegistrations();
		void PerformComponentDeletions();
		void PerformEntityDeletions();
		bool DeleteComponent(Entity entity, const ComponentType* componentType);

	private:
		EntityRegistry m_EntityRegistry;
		EntityPublisher m_EntityPublisher;
		JobScheduler m_JobScheduler;
		ComponentStorage m_ComponentStorage;

		TArray<Entity> m_EntitiesToDelete;
		TArray<std::pair<Entity, const ComponentType*>> m_ComponentsToDelete;
		TArray<std::pair<Entity, const ComponentType*>> m_ComponentsToRegister;

		Timestamp m_DeltaTime;

		SpinLock m_LockAddComponent, m_LockRemoveComponent, m_LockRemoveEntity;

	private:
		static ECSCore* s_pInstance;
	};

	template<typename Comp>
	inline Comp& ECSCore::AddComponent(Entity entity, const Comp& component)
	{
		std::scoped_lock<SpinLock> lock(m_LockAddComponent);
		if (!m_ComponentStorage.HasType<Comp>())
			m_ComponentStorage.RegisterComponentType<Comp>();

		/*	Create component immediately, but hold off on registering and publishing it until the end of the frame.
			This is to prevent concurrency issues. Publishing a component means pushing entity IDs to IDVectors,
			and there is no guarentee that no one is simultaneously reading from these IDVectors. */
		Comp& comp = m_ComponentStorage.AddComponent<Comp>(entity, component);
		m_ComponentsToRegister.PushBack({entity, Comp::Type()});
		return comp;
	}

	template<typename Comp>
	inline Comp& ECSCore::GetComponent(Entity entity)
	{
		return m_ComponentStorage.GetComponent<Comp>(entity);
	}

	template<typename Comp>
	inline const Comp& ECSCore::GetComponent(Entity entity) const
	{
		return m_ComponentStorage.GetComponent<Comp>(entity);
	}

	template<typename Comp>
	inline ComponentArray<Comp>* ECSCore::GetComponentArray()
	{
		return m_ComponentStorage.GetComponentArray<Comp>();
	}

	template<typename Comp>
	inline const ComponentArray<Comp>* ECSCore::GetComponentArray() const
	{
		return m_ComponentStorage.GetComponentArray<Comp>();
	}

	template<typename Comp>
	inline void ECSCore::RemoveComponent(Entity entity)
	{
		std::scoped_lock<SpinLock> lock(m_LockRemoveComponent);
		m_ComponentsToDelete.PushBack({entity, Comp::Type()});
	}
}
