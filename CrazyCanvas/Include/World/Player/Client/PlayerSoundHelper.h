#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"

class PlayerSoundHelper
{
public:
	static void HandleMovementSound(
		const LambdaEngine::VelocityComponent& velocityComponent,
		LambdaEngine::AudibleComponent& audibleComponent,
		bool walking,
		bool inAir,
		bool wasInAir);
};