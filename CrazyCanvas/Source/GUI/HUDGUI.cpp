#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "GUI/HUDGUI.h"
#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/GameOverGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "GUI/GUIHelpers.h"

#include "Game/State.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Match/Match.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "NoesisPCH.h"
#include "States/MainMenuState.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/CommonApplication.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Match/Match.h"

#include "Lobby/PlayerManagerClient.h"

#include <string>

using namespace LambdaEngine;
using namespace Noesis;

HUDGUI::HUDGUI() :
	m_GUIState()
{
	Noesis::GUI::LoadComponent(this, "HUD.xaml");

	InitGUI();
}

HUDGUI::~HUDGUI()
{
	m_PlayerGrids.clear();

	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &HUDGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &HUDGUI::MouseButtonCallback);
}

bool HUDGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// Escape
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonResumeClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonExitClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonChangeKeyBindingsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnVolumeSliderChanged);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSetKey);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplyKeyBindingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelKeyBindingsClick);

	return false;
}

bool HUDGUI::UpdateHealth(int32 currentHealth)
{
	//Returns false if player is dead
	if (currentHealth != m_GUIState.Health)
	{
		Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();

		float healthScale = (float)currentHealth / (float)m_GUIState.MaxHealth;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(healthScale);
		m_pHealthRect->SetRenderTransform(scale);

		std::string hpString = std::to_string((int32)(healthScale * 100)) + "%";
		FrameworkElement::FindName<Noesis::TextBlock>("HEALTH_DISPLAY")->SetText(hpString.c_str());

		m_GUIState.Health = currentHealth;
	}
	return true;
}

bool HUDGUI::UpdateScore()
{
	std::string scoreString;
	uint32 blueScore = Match::GetScore(0);
	uint32 redScore = Match::GetScore(1);


	// poor solution to handle bug if Match being reset before entering

	if (m_GUIState.Scores[0] != blueScore && blueScore != 0)	//Blue
	{
		m_GUIState.Scores[0] = blueScore;

		m_pBlueScoreGrid->GetChildren()->Get(5 - blueScore)->SetVisibility(Visibility::Visibility_Visible);
	}
	else if (m_GUIState.Scores[1] != redScore && redScore != 0) //Red
	{
		m_GUIState.Scores[1] = redScore;

		m_pRedScoreGrid->GetChildren()->Get(redScore - 1)->SetVisibility(Visibility::Visibility_Visible);
	}

	return true;
}

bool HUDGUI::UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType)
{
	//Returns false if Out Of Ammo
	std::string ammoString;
	Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();

	auto ammo = WeaponTypeAmmo.find(ammoType);

	if (ammo != WeaponTypeAmmo.end())
	{
		ammoString = std::to_string(ammo->second.first) + "/" + std::to_string(ammo->second.second);
		float ammoScale = (float)ammo->second.first / (float)ammo->second.second;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(ammoScale);
	}
	else
	{
		LOG_ERROR("Non-existing ammoType");
		return false;
	}


	if (ammoType == EAmmoType::AMMO_TYPE_WATER)
	{
		m_pWaterAmmoText->SetText(ammoString.c_str());
		m_pWaterAmmoRect->SetRenderTransform(scale);
	}
	else if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
	{
		m_pPaintAmmoText->SetText(ammoString.c_str());
		m_pPaintAmmoRect->SetRenderTransform(scale);
	}

	return true;
}

void HUDGUI::ToggleEscapeMenu()
{
	if (Input::GetCurrentInputmode() == EInputLayer::GAME)
	{
		Input::PushInputMode(EInputLayer::GUI);
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);

		m_pEscapeGrid->SetVisibility(Noesis::Visibility_Visible);
		m_ContextStack.push(m_pEscapeGrid);
	}
	else if (Input::GetCurrentInputmode() == EInputLayer::GUI)
	{
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
		Noesis::FrameworkElement* pElement = m_ContextStack.top();
		pElement->SetVisibility(Noesis::Visibility_Collapsed);
		m_ContextStack.pop();
		Input::PopInputMode();
	}
}

void HUDGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_ContextStack.pop();
	Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
	pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
}

void HUDGUI::OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	m_MouseEnabled = !m_MouseEnabled;
	CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Collapsed);
	m_ContextStack.pop();
	Input::PopInputMode();
}

void HUDGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pSettingsGrid);
}

void HUDGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	ClientHelper::Disconnect("Left by choice");
	SetRenderStagesInactive();

	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Collapsed);
	m_ContextStack.pop();

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	Input::PopInputMode();
}

void HUDGUI::OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	CommonApplication::Get()->Terminate();
}

void HUDGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// Ray Tracing
	Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING, m_RayTracingEnabled);

	// Mesh Shader
	Noesis::CheckBox* pMeshShaderCheckBox = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	m_MeshShadersEnabled = pMeshShaderCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, m_MeshShadersEnabled);

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER, volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	EngineConfig::WriteToFile();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonChangeKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pKeyBindingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pKeyBindingsGrid);
}

void HUDGUI::OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	// Update volume for easier changing of it. Do not save it however as that should
	// only be done when the user presses "Apply"

	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	AudioAPI::GetDevice()->SetMasterVolume(volume);
}

void HUDGUI::OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// Starts listening to callbacks with specific button to be changed. This action is deferred to
	// the callback functions of KeyboardCallback and MouseButtonCallback.

	Noesis::Button* pCalledButton = static_cast<Noesis::Button*>(pSender);
	LambdaEngine::String buttonName = pCalledButton->GetName();

	m_pSetKeyButton = FrameworkElement::FindName<Button>(buttonName.c_str());
	m_ListenToCallbacks = true;
}

void HUDGUI::OnButtonApplyKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Go through all keys to set - and set them
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	for (auto& stringPair : m_KeysToSet)
	{
		InputActionSystem::ChangeKeyBinding(StringToAction(stringPair.first), stringPair.second);
	}
	m_KeysToSet.clear();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonCancelKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Reset
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	for (auto& stringPair : m_KeysToSet)
	{
		EAction action = StringToAction(stringPair.first);
		EKey key = InputActionSystem::GetKey(action);
		EMouseButton mouseButton = InputActionSystem::GetMouseButton(action);

		if (key != EKey::KEY_UNKNOWN)
		{
			LambdaEngine::String keyStr = KeyToString(key);
			FrameworkElement::FindName<Button>(stringPair.first.c_str())->SetContent(keyStr.c_str());
		}
		if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			LambdaEngine::String mouseButtonStr = ButtonToString(mouseButton);
			FrameworkElement::FindName<Button>(stringPair.first.c_str())->SetContent(mouseButtonStr.c_str());
		}
	}
	m_KeysToSet.clear();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::UpdateCountdown(uint8 countDownTime)
{
	CountdownGUI* pCountdownGUI = FindName<CountdownGUI>("COUNTDOWN");
	pCountdownGUI->UpdateCountdown(countDownTime);
}

void HUDGUI::DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal)
{
	Noesis::Ptr<Noesis::RotateTransform> rotateTransform = *new RotateTransform();

	glm::vec3 forwardDir = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
	glm::vec3 nor = glm::normalize(glm::vec3(collisionNormal.x, 0.0f, collisionNormal.z));

	float32 result = glm::dot(forwardDir, nor);
	float32 rotation = 0.0f;


	if (result > 0.99f)
	{
		rotation = 0.0f;
	}
	else if (result < -0.99f)
	{
		rotation = 180.0f;
	}
	else
	{
		glm::vec3 res = glm::cross(forwardDir, nor);

		rotation = glm::degrees(glm::acos(glm::dot(forwardDir, nor)));

		if (res.y > 0)
		{
			rotation *= -1;
		}
	}

	rotateTransform->SetAngle(rotation);
	m_pHitIndicatorGrid->SetRenderTransform(rotateTransform);

	DamageIndicatorGUI* pDamageIndicatorGUI = FindName<DamageIndicatorGUI>("DAMAGE_INDICATOR");
	pDamageIndicatorGUI->DisplayIndicator();
}

void HUDGUI::DisplayHitIndicator()
{
	EnemyHitIndicatorGUI* pEnemyHitIndicatorGUI = FindName<EnemyHitIndicatorGUI>("HIT_INDICATOR");
	pEnemyHitIndicatorGUI->DisplayIndicator();
}

void HUDGUI::DisplayScoreboardMenu(bool visible)
{
	// Toggle to visible
	if (!m_ScoreboardVisible && visible)
	{
		m_ScoreboardVisible = true;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Visible);
	}
	// Toggle to hidden
	else if (m_ScoreboardVisible && !visible)
	{
		m_ScoreboardVisible = false;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Hidden);
	}
}

void HUDGUI::AddPlayer(const Player& newPlayer)
{
	Ptr<Grid> pGrid = *new Grid();
	m_PlayerGrids[newPlayer.GetUID()] = pGrid;

	pGrid->SetName(std::to_string(newPlayer.GetUID()).c_str());
	FrameworkElement::GetView()->GetContent()->RegisterName(pGrid->GetName(), pGrid);

	ColumnDefinitionCollection* pColumnCollection = pGrid->GetColumnDefinitions();
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 2.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);

	// Name is different than other labels
	Ptr<Label> pNameLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pNameLabel->SetContent(newPlayer.GetName().c_str());
	pNameLabel->SetForeground(pWhiteBrush);
	pNameLabel->SetFontSize(28.f);
	pNameLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	uint8 column = 1;
	pGrid->GetChildren()->Add(pNameLabel);
	pGrid->SetColumn(pNameLabel, column++);

	AddStatsLabel(pGrid, std::to_string(newPlayer.GetKills()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetDeaths()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsCaptured()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsDefended()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetPing()), column++);

	if (newPlayer.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else if (newPlayer.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else
	{
		LOG_WARNING("[HUDGUI]: Unknown team on player \"%s\".\n\tUID: %lu\n\tTeam: %d",
			newPlayer.GetName().c_str(), newPlayer.GetUID(), newPlayer.GetTeam());
	}

	if (PlayerManagerClient::GetPlayerLocal()->GetUID() == newPlayer.GetUID())
	{
		Ptr<Image> localPlayerIcon = *new Image();
		Ptr<BitmapImage> srcImage = *new BitmapImage();
		srcImage->SetUriSource(Noesis::Uri::Uri("splashes/splash_green.png"));
		localPlayerIcon->SetSource(srcImage);
		localPlayerIcon->SetWidth(40);
		localPlayerIcon->SetHeight(40);
		pGrid->GetChildren()->Add(localPlayerIcon);
		pGrid->SetColumn(localPlayerIcon, 0);
	}
}

void HUDGUI::RemovePlayer(const Player& player)
{
	if (!m_PlayerGrids.contains(player.GetUID()))
	{
		LOG_WARNING("[HUDGUI]: Tried to delete \"%s\", but could not find player UID.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}
	Grid* pGrid = m_PlayerGrids[player.GetUID()];

	if (!pGrid)
	{
		LOG_WARNING("[HUDGUI]: Tried to delete \"%s\", but could not find grid.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}

	m_PlayerGrids.erase(player.GetUID());

	if (player.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(pGrid);
	}
	else if (player.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(pGrid);
	}
	else
	{
		LOG_WARNING("[HUDGUI]: Tried to remove player with unknown team. Playername: %s, team: %d", player.GetName().c_str(), player.GetTeam());
	}

}

void HUDGUI::UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value)
{
	uint8 index = 0;
	switch (property)
	{
	case EPlayerProperty::PLAYER_PROPERTY_NAME:				index = 0; break;
	case EPlayerProperty::PLAYER_PROPERTY_KILLS:			index = 1; break;
	case EPlayerProperty::PLAYER_PROPERTY_DEATHS:			index = 2; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED:	index = 3; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED:	index = 4; break;
	case EPlayerProperty::PLAYER_PROPERTY_PING:				index = 5; break;
	default: LOG_WARNING("[HUDGUI]: Enum not supported"); return;
	}

	if (!m_PlayerGrids.contains(playerUID))
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", playerUID);
		return;
	}
	Grid* pGrid = m_PlayerGrids[playerUID];

	Label* pLabel = static_cast<Label*>(pGrid->GetChildren()->Get(index));
	pLabel->SetContent(value.c_str());
}

void HUDGUI::UpdateAllPlayerProperties(const Player& player)
{
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_NAME, player.GetName());
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_KILLS, std::to_string(player.GetKills()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_DEATHS, std::to_string(player.GetDeaths()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED, std::to_string(player.GetFlagsCaptured()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED, std::to_string(player.GetFlagsDefended()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_PING, std::to_string(player.GetPing()));
}

void HUDGUI::AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column)
{
	Ptr<Label> pLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pLabel->SetContent(content.c_str());
	pLabel->SetForeground(pWhiteBrush);
	pLabel->SetHorizontalAlignment(HorizontalAlignment::HorizontalAlignment_Right);
	pLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	pParentGrid->GetChildren()->Add(pLabel);
	pParentGrid->SetColumn(pLabel, column);
}

void HUDGUI::UpdatePlayerAliveStatus(uint64 UID, bool isAlive)
{
	if (!m_PlayerGrids.contains(UID))
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", UID);
		return;
	}
	Grid* pGrid = m_PlayerGrids[UID];

	Label* nameLabel = static_cast<Label*>(pGrid->GetChildren()->Get(0));

	LOG_ERROR("[HUDGUI]: Name: %s", nameLabel->GetContent());

	SolidColorBrush* pBrush = static_cast<SolidColorBrush*>(nameLabel->GetForeground());
	if (isAlive)
	{
		pBrush->SetColor(Color::White());
		nameLabel->SetForeground(pBrush);
	}
	else
	{
		pBrush->SetColor(Color::LightGray());
		nameLabel->SetForeground(pBrush);
	}
}

void HUDGUI::ProjectGUIIndicator(const glm::mat4& viewProj, const glm::vec3& worldPos, Entity entity)
{

	Noesis::Ptr<Noesis::TranslateTransform> translation = *new TranslateTransform();

	const glm::vec4 clipSpacePos = viewProj * glm::vec4(worldPos, 1.0f);

	VALIDATE(clipSpacePos.w != 0);

	const glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos.x, clipSpacePos.y, clipSpacePos.z) / clipSpacePos.w;
	const glm::vec2 windowSpacePos = glm::vec2(ndcSpacePos.x, -ndcSpacePos.y) * 0.5f * m_WindowSize;

	float32 vecLength = glm::distance(glm::vec2(0.0f), glm::vec2(ndcSpacePos.x, ndcSpacePos.y));

	if (clipSpacePos.z > 0)
	{
		translation->SetY(glm::clamp(windowSpacePos.y, -m_WindowSize.y * 0.5f, m_WindowSize.y * 0.5f));
		translation->SetX(glm::clamp(windowSpacePos.x, -m_WindowSize.x * 0.5f, m_WindowSize.x * 0.5f));
		SetIndicatorOpacity(glm::max(0.1f, vecLength), entity);
	}
	else
	{
		if (-clipSpacePos.y > 0)
			translation->SetY(m_WindowSize.y * 0.5f);
		else
			translation->SetY(-m_WindowSize.y * 0.5f);

		translation->SetX(glm::clamp(-windowSpacePos.x, -m_WindowSize.x * 0.5f, m_WindowSize.x * 0.5f));
	}
	
	TranslateIndicator(translation, entity);
}

void HUDGUI::SetWindowSize(uint32 width, uint32 height)
{
	m_WindowSize = glm::vec2(width, height);
}

void HUDGUI::DisplayGameOverGrid(uint8 winningTeamIndex, PlayerPair& mostKills, PlayerPair& mostDeaths, PlayerPair& mostFlags)
{
	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Hidden);

	GameOverGUI* pGameOverGUI = FindName<GameOverGUI>("GAME_OVER");
	pGameOverGUI->InitGUI();
	pGameOverGUI->DisplayGameOverGrid(true);
	pGameOverGUI->SetWinningTeam(winningTeamIndex);

	pGameOverGUI->SetMostKillsStats(mostKills.first, mostKills.second->GetName());
	pGameOverGUI->SetMostDeathsStats(mostDeaths.first, mostDeaths.second->GetName());
	pGameOverGUI->SetMostFlagsStats(mostFlags.first, mostFlags.second->GetName());
}

void HUDGUI::InitGUI()
{
	m_GUIState.Health			= m_GUIState.MaxHealth;
	m_GUIState.AmmoCapacity		= 50;
	m_GUIState.Ammo				= m_GUIState.AmmoCapacity;

	m_GUIState.Scores.PushBack(Match::GetScore(0));
	m_GUIState.Scores.PushBack(Match::GetScore(1));

	m_pWaterAmmoRect	= FrameworkElement::FindName<Image>("WATER_RECT");
	m_pPaintAmmoRect	= FrameworkElement::FindName<Image>("PAINT_RECT");
	m_pHealthRect		= FrameworkElement::FindName<Image>("HEALTH_RECT");

	m_pWaterAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_WATER_DISPLAY");
	m_pPaintAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_PAINT_DISPLAY");

	m_pHitIndicatorGrid	= FrameworkElement::FindName<Grid>("DAMAGE_INDICATOR_GRID");
	m_pScoreboardGrid	= FrameworkElement::FindName<Grid>("SCOREBOARD_GRID");

	m_pRedScoreGrid		= FrameworkElement::FindName<Grid>("RED_TEAM_SCORE_GRID");
	m_pBlueScoreGrid	= FrameworkElement::FindName<Grid>("BLUE_TEAM_SCORE_GRID");

	m_pBlueTeamStackPanel	= FrameworkElement::FindName<StackPanel>("BLUE_TEAM_STACK_PANEL");
	m_pRedTeamStackPanel	= FrameworkElement::FindName<StackPanel>("RED_TEAM_STACK_PANEL");

	m_pHUDGrid = FrameworkElement::FindName<Grid>("ROOT_CONTAINER");

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);

	m_pWaterAmmoText->SetText(ammoString.c_str());
	m_pPaintAmmoText->SetText(ammoString.c_str());

	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Visible);
	CommonApplication::Get()->SetMouseVisibility(false);

	m_WindowSize.x = CommonApplication::Get()->GetMainWindow()->GetWidth();
	m_WindowSize.y = CommonApplication::Get()->GetMainWindow()->GetHeight();

	// Main Grids
	m_pEscapeGrid			= FrameworkElement::FindName<Grid>("EscapeGrid");
	m_pSettingsGrid			= FrameworkElement::FindName<Grid>("SettingsGrid");
	m_pKeyBindingsGrid		= FrameworkElement::FindName<Grid>("KeyBindingsGrid");

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &HUDGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &HUDGUI::MouseButtonCallback);

	SetDefaultSettings();
}

void HUDGUI::SetDefaultSettings()
{
	// Set inital volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(pVolumeSlider);
	float volume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER);
	pVolumeSlider->SetValue(volume * pVolumeSlider->GetMaximum());
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	SetDefaultKeyBindings();

	// Ray Tracing Toggle
	m_RayTracingEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);
	CheckBox* pToggleRayTracing = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	NS_ASSERT(pToggleRayTracing);
	pToggleRayTracing->SetIsChecked(m_RayTracingEnabled);

	// Mesh Shader Toggle
	m_MeshShadersEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);
	ToggleButton* pToggleMeshShader = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);
}

void HUDGUI::SetDefaultKeyBindings()
{
	TArray<EAction> actions = {
		// Movement
		EAction::ACTION_MOVE_FORWARD,
		EAction::ACTION_MOVE_BACKWARD,
		EAction::ACTION_MOVE_LEFT,
		EAction::ACTION_MOVE_RIGHT,
		EAction::ACTION_MOVE_JUMP,
		EAction::ACTION_MOVE_WALK,

		// Attack
		EAction::ACTION_ATTACK_PRIMARY,
		EAction::ACTION_ATTACK_SECONDARY,
		EAction::ACTION_ATTACK_RELOAD,
	};

	for (EAction action : actions)
	{
		EKey key = InputActionSystem::GetKey(action);
		EMouseButton mouseButton = InputActionSystem::GetMouseButton(action);

		if (key != EKey::KEY_UNKNOWN)
		{
			FrameworkElement::FindName<Button>(ActionToString(action))->SetContent(KeyToString(key));
		}
		else if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			FrameworkElement::FindName<Button>(ActionToString(action))->SetContent(ButtonToString(mouseButton));
		}
	}
}

void HUDGUI::SetRenderStagesInactive()
{
	/*
	* Inactivate all rendering when entering main menu
	*OBS! At the moment, sleeping doesn't work correctly and needs a fix
	* */
	DisablePlaySessionsRenderstages();
}

bool HUDGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String keyStr = KeyToString(event.Key);

		m_pSetKeyButton->SetContent(keyStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = keyStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}
	else if (event.Key == EKey::KEY_ESCAPE)
	{
		ToggleEscapeMenu();

		return true;
	}

	return false;
}

bool HUDGUI::MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String mouseButtonStr = ButtonToString(event.Button);

		m_pSetKeyButton->SetContent(mouseButtonStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = mouseButtonStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}

	return false;
}

void HUDGUI::CreateProjectedGUIElement(Entity entity, uint8 localTeamIndex, uint8 teamIndex)
{
	Noesis::Ptr<Noesis::Rectangle> indicator = *new Noesis::Rectangle();
	Noesis::Ptr<Noesis::TranslateTransform> translation = *new TranslateTransform();

	translation->SetY(100.0f);
	translation->SetX(100.0f);

	indicator->SetRenderTransform(translation);
	indicator->SetRenderTransformOrigin(Noesis::Point(0.5f, 0.5f));

	Ptr<Noesis::SolidColorBrush> brush = *new Noesis::SolidColorBrush();

	if (teamIndex != UINT8_MAX)
	{
		if (localTeamIndex == teamIndex)
			brush->SetColor(Noesis::Color::Blue());
		else
			brush->SetColor(Noesis::Color::Red());
	}
	else
		brush->SetColor(Noesis::Color::Green());

	indicator->SetHeight(40);
	indicator->SetWidth(40);

	indicator->SetFill(brush);

	m_ProjectedElements[entity] = indicator;

	if (m_pHUDGrid->GetChildren()->Add(indicator) == -1)
	{
		LOG_ERROR("Could not add Proj Element");
	}
}

void HUDGUI::RemoveProjectedGUIElement(LambdaEngine::Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	m_pHUDGrid->GetChildren()->Remove(indicator->second);

	m_ProjectedElements.erase(indicator->first);
}

void HUDGUI::TranslateIndicator(Noesis::Transform* pTranslation, Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	indicator->second->SetRenderTransform(pTranslation);
}

void HUDGUI::SetIndicatorOpacity(float32 value, Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	indicator->second->GetFill()->SetOpacity(value);
}
