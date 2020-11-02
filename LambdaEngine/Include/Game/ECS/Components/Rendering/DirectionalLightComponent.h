#pragma once


#include "ECS/Component.h"

namespace LambdaEngine
{
	struct DirectionalLightComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(DirectionalLightComponent);
		glm::vec4	ColorIntensity	= glm::vec4(1.0f, 1.0f, 1.0f, 25.f);
		float		FrustumWidth	= 15.0f;
		float		FrustumHeight	= 15.0f;
		float		FrustumZNear	= -30.0f;
		float		FrustumZFar		= 10.0f;
	};
}