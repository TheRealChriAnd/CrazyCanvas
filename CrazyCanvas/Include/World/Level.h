#pragma once

#include "LambdaEngine.h"

#include "LevelModule.h"
#include "Game/World/LevelObjectCreator.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/String.h"

#include "ECS/Entity.h"

struct LevelCreateDesc
{
	LambdaEngine::String Name	=	"";
	LambdaEngine::TArray<LevelModule*>		LevelModules;
};

class Level
{
public:
	DECL_UNIQUE_CLASS(Level);

	Level();
	~Level();

	bool Init(const LevelCreateDesc* pDesc);

	// This should be used to create server entities:
	// Server & Client Loads Level, Client only loads clientside level objects,
	// Server then sends packages about server side entities that need to be created in the client, those should use this method
	// This method should delegate to LevelObjectCreator
	bool CreateObject(LambdaEngine::ESpecialObjectType specialObjectType, void* pData);

	/*
	*	Spawns a player at a random Spawnpoint, the player is forced to be local
	*/
	void SpawnPlayer(
		const LambdaEngine::MeshComponent& meshComponent,
		const LambdaEngine::AnimationComponent& animationComponent,
		const LambdaEngine::CameraDesc* pCameraDesc);

	uint32 GetEntityCount(LambdaEngine::ESpecialObjectType specialObjectType) const;

private:
	LambdaEngine::String m_Name = "";
	LambdaEngine::THashTable<LambdaEngine::ESpecialObjectType, LambdaEngine::TArray<uint32>> m_EntityTypeMap;
	LambdaEngine::TArray<LambdaEngine::Entity> m_Entities;
};