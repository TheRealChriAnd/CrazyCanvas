#pragma once
#include "LambdaEngine.h"

#include "ECS/System.h"

#include "Resources/Mesh.h"

namespace LambdaEngine
{
	struct MeshComponent;
	struct AnimationComponent;

	class AnimationSystem : public System
	{
	public:
		bool Init();

		void OnEntityAdded(Entity entity);
		void OnEntityRemoved(Entity entity);

		// Inherited from system
		virtual void Tick(Timestamp deltaTime) override final;

	private:
		AnimationSystem();
		~AnimationSystem();

		void Animate(Timestamp deltaTime, AnimationComponent& animation, MeshComponent& mesh);
		glm::mat4 ApplyParent(Skeleton::Bone& bone, Skeleton& skeleton, TArray<glm::mat4>& matrices);

	public:
		static AnimationSystem& GetInstance();

	private:
		IDVector m_AnimationEntities;
	};
}