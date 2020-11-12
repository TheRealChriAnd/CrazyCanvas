#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/Image.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/StackPanel.h"
#include "NsGui/ObservableCollection.h"
#include "NsGui/Button.h"

#include "Lobby/PlayerManagerBase.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "Lobby/Player.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#define MAX_AMMO 100

struct GameGUIState
{
	int32 Health;
	int32 MaxHealth = 100;

	LambdaEngine::TArray<uint32> Scores;

	int32 Ammo;
	int32 AmmoCapacity;
};

enum class EPlayerProperty
{
	PLAYER_PROPERTY_NAME,
	PLAYER_PROPERTY_KILLS,
	PLAYER_PROPERTY_DEATHS,
	PLAYER_PROPERTY_FLAGS_CAPTURED,
	PLAYER_PROPERTY_FLAGS_DEFENDED,
	PLAYER_PROPERTY_PING,
};
typedef  std::pair<uint8, const Player*> PlayerPair;

class HUDGUI : public Noesis::Grid
{
public:
	HUDGUI();
	~HUDGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);
	bool OpenEscapeMenu(const LambdaEngine::KeyPressedEvent& event);

	// Escape GUI
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// Settings
	void OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonChangeKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// Key bindings
	void OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplyKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void UpdateCountdown(uint8 countDownTime);

	void DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal);
	void DisplayHitIndicator();
	void DisplayScoreboardMenu(bool visible);
	void DisplayGameOverGrid(uint8 winningTeamIndex, PlayerPair& mostKills, PlayerPair& mostDeaths, PlayerPair& mostFlags);

	void AddPlayer(const Player& newPlayer);
	void RemovePlayer(const Player& player);
	void UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value);
	void UpdateAllPlayerProperties(const Player& player);
	void UpdatePlayerAliveStatus(uint64 UID, bool isAlive);

private:
	void InitGUI();

	void SetDefaultSettings();
	void SetDefaultKeyBindings();
	void SetRenderStagesInactive();
	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);

	void AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column);
	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;
	bool m_IsGameOver = false;

	Noesis::Image* m_pWaterAmmoRect = nullptr;
	Noesis::Image* m_pPaintAmmoRect = nullptr;
	Noesis::Image* m_pHealthRect = nullptr;
	
	Noesis::TextBlock* m_pWaterAmmoText = nullptr;
	Noesis::TextBlock* m_pPaintAmmoText = nullptr;

	Noesis::Grid* m_pHitIndicatorGrid	= nullptr;
	Noesis::Grid* m_pScoreboardGrid		= nullptr;

	Noesis::StackPanel* m_pBlueTeamStackPanel	= nullptr;
	Noesis::StackPanel* m_pRedTeamStackPanel	= nullptr;

	LambdaEngine::THashTable<uint64, Noesis::Grid*> m_PlayerGrids;

	bool m_ScoreboardVisible = false;
	// EscapeGUI
	bool 			m_ListenToCallbacks		= false;
	Noesis::Button* m_pSetKeyButton			= nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;

	bool			m_RayTracingEnabled		= false;
	bool			m_MeshShadersEnabled	= false;
	bool			m_EscapeMenuEnabled		= false;

	bool			m_MouseEnabled			= false;

	Noesis::Grid*	m_pEscapeGrid			= nullptr;
	Noesis::Grid*	m_pSettingsGrid			= nullptr;
	Noesis::Grid*	m_pKeyBindingsGrid		= nullptr;

	LambdaEngine::TStack<Noesis::FrameworkElement*> m_ContextStack;

	Noesis::Grid* m_pHitIndicatorGrid = nullptr;
};