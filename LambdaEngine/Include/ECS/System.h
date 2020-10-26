#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/RegularWorker.h"

#include "Time/API/Timestamp.h"

#include <functional>
#include <typeindex>

namespace LambdaEngine
{
	struct SystemRegistration
	{
		EntitySubscriberRegistration SubscriberRegistration;
		uint32 Phase = 0;
		uint32 TickFrequency = 0;
	};

	class ComponentHandler;

	// A system processes components each frame in the tick function
	class LAMBDA_API System : private EntitySubscriber, private RegularWorker
	{
	public:
		// Registers the system in the system handler
		System() = default;

		// Deregisters system
		virtual ~System() = default;

		virtual void Tick(Timestamp deltaTime) = 0;

	protected:
		void RegisterSystem(SystemRegistration& systemRegistration);
	};
}
