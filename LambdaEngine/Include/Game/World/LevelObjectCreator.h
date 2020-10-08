#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include "Resources/ResourceLoader.h"

#include "ECS/Entity.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"

namespace LambdaEngine
{
	class CameraDesc;

	enum class ESpecialObjectType : uint8
	{
		SPECIAL_OBJECT_TYPE_NONE		= 0,
		SPECIAL_OBJECT_TYPE_SPAWN_POINT	= 1,
		SPECIAL_OBJECT_TYPE_FLAG		= 2,
		SPECIAL_OBJECT_TYPE_PLAYER		= 3,
	};

	struct CreatePlayerDesc
	{
		bool				IsLocal				= false;
		glm::vec3			Position			= glm::vec3(0.0f);
		glm::vec3			Forward				= glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3			Scale				= glm::vec3(1.0f);
		const CameraDesc*	pCameraDesc			= nullptr;
		MeshComponent		MeshComponent;
		AnimationComponent	AnimationComponent;
	};

	class LevelObjectCreator
	{
		typedef ESpecialObjectType(*SpecialObjectCreateByPrefixFunc)(const SpecialObjectOnLoad&, TArray<Entity>&, const glm::vec3&);
		typedef bool(*SpecialObjectCreateByTypeFunc)(const void* pData, TArray<Entity>&);

		static constexpr const float PLAYER_CAPSULE_HEIGHT = 1.0f;
		static constexpr const float PLAYER_CAPSULE_RADIUS = 0.2f;

	public:
		DECL_STATIC_CLASS(LevelObjectCreator);

		static bool Init(bool clientSide);

		static Entity CreateDirectionalLight(const LoadedDirectionalLight& directionalLight, const glm::vec3& translation);
		static Entity CreatePointLight(const LoadedPointLight& pointLight, const glm::vec3& translation);
		static Entity CreateStaticGeometry(const MeshComponent& meshComponent, const glm::vec3& translation);

		/*
		*	Special Objects are entities that can be created at level load time or later, they are more than general than lights and geometry in the sense
		*	that as long as a create function exists, it can be created. This means that in order to add a new type of Special Object, one just has to add a new
		*	a new create function for that Special Object.
		* 
		*	The create functions can then either be called at level load time using a prefix which can be registered in LevelObjectCreator::Init and must be set
		*	on the appropriate mesh, yes mesh (blame assimp), in the Level Editor. Or it can be created by registering the create function in s_SpecialObjectTypeCreateFunctions
		*	and later calling Level::CreateObject with the appropriate SpecialObjectType and pData, this will in turn call LevelObjectCreator::CreateSpecialObjectOfType.
		* 
		*	Special Objects are similar to ECS::Entities but one Special Object can create many entities.
		*/
		static ESpecialObjectType CreateSpecialObjectFromPrefix(const SpecialObjectOnLoad& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);
		static bool CreateSpecialObjectOfType(ESpecialObjectType specialObjectType, const void* pData, TArray<Entity>& createdEntities);

		FORCEINLINE static const TArray<SpecialObjectOnLoadDesc>& GetSpecialObjectOnLoadDescriptions() { return s_SpecialObjectOnLoadDescriptions; }

	private:
		static ESpecialObjectType CreateSpawnpoint(const SpecialObjectOnLoad& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);
		static ESpecialObjectType CreateFlag(const SpecialObjectOnLoad& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);

		static bool CreatePlayer(const void* pData, TArray<Entity>& createdEntities);

	private:
		inline static bool m_ClientSide = false;
		inline static TArray<SpecialObjectOnLoadDesc> s_SpecialObjectOnLoadDescriptions;
		inline static THashTable<String, SpecialObjectCreateByPrefixFunc> s_SpecialObjectByPrefixCreateFunctions;
		inline static THashTable<ESpecialObjectType, SpecialObjectCreateByTypeFunc> s_SpecialObjectByTypeCreateFunctions;
	};
}