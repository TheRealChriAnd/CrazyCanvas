#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct ControllableComponent
	{
		DECL_COMPONENT(ControllableComponent);
		bool IsActive = false;

		glm::vec3 StartPosition	= glm::vec3(0.0f);
		glm::vec3 EndPosition	= glm::vec3(0.0f);

		Timestamp StartTimestamp	= 0;
		Timestamp Duration			= 0;
	};
}