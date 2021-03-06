#pragma once
#include "Game/State.h"

#include "ECS/System.h"

#include "Physics/PhysicsEvents.h"
#include "Events/GameplayEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/MatchEvents.h"
#include "Application/API/Events/WindowEvents.h"

#include "GUI/HUDGUI.h"

#include "Multiplayer/Packet/PacketTeamScored.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"



class HUDSystem : public LambdaEngine::System
{
public:
	HUDSystem() = default;
	~HUDSystem();

	void Init();

	/*void Resume() override final {};
	void Pause() override final {};
	*/
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;
	void FixedTick(LambdaEngine::Timestamp delta);

	bool OnWeaponFired(const WeaponFiredEvent& event);
	bool OnWeaponReloadFinished(const WeaponReloadFinishedEvent& event);
	bool OnWeaponReloadStartedEvent(const WeaponReloadStartedEvent& event);
	bool OnWeaponReloadCanceledEvent(const WeaponReloadCanceledEvent& event);
	bool OnFlagPickedUpEvent(const FlagPickedUpEvent& event);
	bool OnFlagDroppedEvent(const FlagDroppedEvent& event);
	bool OnProjectileHit(const ProjectileHitEvent& event);
	bool OnSpectatePlayerEvent(const SpectatePlayerEvent& event);
	bool OnPlayerScoreUpdated(const PlayerScoreUpdatedEvent& event);
	bool OnPlayerPingUpdated(const PlayerPingUpdatedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);
	bool OnGameOver(const GameOverEvent& event);
	bool OnWindowResized(const LambdaEngine::WindowResizedEvent& event);

	void PromptMessage(const LambdaEngine::String& promtMessage, bool isSmallPrompt, uint8 teamIndex = UINT8_MAX);

	void DisplayNamePlate(const LambdaEngine::String& name, bool isLookingAtTeamPlayer);

private:
	bool OnMatchCountdownEvent(const MatchCountdownEvent& event);
	bool OnPacketTeamScored(const PacketReceivedEvent<PacketTeamScored>& event);
	void OnProjectedEntityAdded(LambdaEngine::Entity projectedEntity);
	void RemoveProjectedEntity(LambdaEngine::Entity projectedEntity);

private:

	LambdaEngine::IDVector m_LocalPlayerEntities;
	LambdaEngine::IDVector m_ForeignPlayerEntities;
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_ProjectedGUIEntities;
	LambdaEngine::IDVector m_CameraEntities;

	Noesis::Ptr<HUDGUI> m_HUDGUI;
	Noesis::Ptr<Noesis::IView> m_View;

	LambdaEngine::SpinLock m_DeferredEventsLock;
	LambdaEngine::TArray<std::pair<ProjectileHitEvent, uint8>> m_DeferredDamageTakenHitEvents;
	LambdaEngine::TArray<std::pair<ProjectileHitEvent, uint8>> m_DamageTakenEventsToProcess;

	LambdaEngine::TArray<bool> m_DeferredEnemyHitEvents;
	LambdaEngine::TArray<bool> m_EnemyHitEventsToProcess;

	uint8 m_LocalTeamIndex = UINT8_MAX;
};
