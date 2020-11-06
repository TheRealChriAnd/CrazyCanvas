#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Lobby/PlayerManagerClient.h"
#include "Multiplayer/ClientHelper.h"

#include "Containers/String.h"

#include "Game/StateManager.h"

#include "States/MultiplayerState.h"

#include "Multiplayer/ServerHostHelper.h"
#include "Multiplayer/ClientHelper.h"

using namespace Noesis;
using namespace LambdaEngine;

LobbyGUI::LobbyGUI()
{
	Noesis::GUI::LoadComponent(this, "Lobby.xaml");

	// Get commonly used elements
	m_pBlueTeamStackPanel	= FrameworkElement::FindName<StackPanel>("BlueTeamStackPanel");
	m_pRedTeamStackPanel	= FrameworkElement::FindName<StackPanel>("RedTeamStackPanel");
	m_pChatPanel			= FrameworkElement::FindName<StackPanel>("ChatStackPanel");
	m_pChatInputTextBox		= FrameworkElement::FindName<TextBox>("ChatInputTextBox");

	m_GameSettings.AuthenticationID = ServerHostHelper::GetAuthenticationHostID();
}

LobbyGUI::~LobbyGUI()
{
}

void LobbyGUI::AddPlayer(const Player& player)
{
	StackPanel* pnl = player.GetTeam() == 0 ? m_pBlueTeamStackPanel : m_pRedTeamStackPanel;

	const LambdaEngine::String& name = player.GetName();

	// Grid
	Ptr<Grid> playerGrid = *new Grid();
	playerGrid->SetName((name + "_grid").c_str());
	RegisterName(name + "_grid", playerGrid);
	ColumnDefinitionCollection* columnCollection = playerGrid->GetColumnDefinitions();
	AddColumnDefinitionStar(columnCollection, 1.f);
	AddColumnDefinitionStar(columnCollection, 7.f);
	AddColumnDefinitionStar(columnCollection, 2.f);

	// Player label
	AddLabelWithStyle(name + "_name", playerGrid, "PlayerTeamLabelStyle", name);

	// Ping label
	AddLabelWithStyle(name + "_ping", playerGrid, "PingLabelStyle", "?");

	// Checkmark image
	Ptr<Image> image = *new Image();
	image->SetName((name + "_checkmark").c_str());
	RegisterName(name + "_checkmark", image);
	Style* style = FrameworkElement::FindResource<Style>("CheckmarkImageStyle");
	image->SetStyle(style);
	image->SetVisibility(Visibility::Visibility_Hidden);
	playerGrid->GetChildren()->Add(image);

	pnl->GetChildren()->Add(playerGrid);
}

void LobbyGUI::RemovePlayer(const Player& player)
{
	const LambdaEngine::String& name = player.GetName();

	Grid* grid = m_pBlueTeamStackPanel->FindName<Grid>((name + "_grid").c_str());
	if (grid)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(grid);
		return;
	}

	grid = m_pRedTeamStackPanel->FindName<Grid>((name + "_grid").c_str());
	if (grid)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(grid);
	}
}

void LobbyGUI::UpdatePlayerPing(const Player& player)
{
	Label* pingLabel = FrameworkElement::FindName<Label>((player.GetName() + "_ping").c_str());
	if (pingLabel)
	{
		pingLabel->SetContent(std::to_string(player.GetPing()).c_str());
	}
}

void LobbyGUI::UpdatePlayerReady(const Player& player)
{
	// Checkmark styling is currently broken
	bool ready = player.GetState() == EPlayerState::PLAYER_STATE_READY;
	Image* image = FrameworkElement::FindName<Image>((player.GetName() + "_checkmark").c_str());
	if (image)
	{
		image->SetVisibility(ready ? Visibility::Visibility_Visible : Visibility::Visibility_Hidden);
	}
}

void LobbyGUI::WriteChatMessage(const ChatEvent& event)
{
	const ChatMessage& chatMessage = event.Message;

	const LambdaEngine::String& name = event.IsSystemMessage() ? "Server" : chatMessage.Name;

	Ptr<DockPanel> dockPanel = *new DockPanel();

	AddLabelWithStyle("", dockPanel, "ChatNameLabelStyle", name);
	AddLabelWithStyle("", dockPanel, "ChatNameSeperatorStyle", "");

	Ptr<TextBox> message = *new TextBox();
	message->SetText(chatMessage.Message.c_str());
	Style* style = FrameworkElement::FindResource<Style>("ChatMessageStyle");
	message->SetStyle(style);
	dockPanel->GetChildren()->Add(message);

	m_pChatPanel->GetChildren()->Add(dockPanel);
}

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonReadyClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSendMessageClick);

	return false;
}

void LobbyGUI::OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	PlayerManagerClient::SetLocalPlayerReady(true);
}

void LobbyGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ClientHelper::Disconnect("Leaving lobby");

	State* pMainMenuState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	LambdaEngine::String message = m_pChatInputTextBox->GetText();
	if (message.empty())
		return;

	m_pChatInputTextBox->SetText("");
	ChatManager::SendChatMessage(message);
}

void LobbyGUI::SendGameSettings()
{
	ClientHelper::Send(m_GameSettings);
}

void LobbyGUI::AddColumnDefinitionStar(ColumnDefinitionCollection* columnCollection, float width)
{
	GridLength gl = GridLength(1.f, GridUnitType::GridUnitType_Star);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	columnCollection->Add(col);
}

void LobbyGUI::AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* parent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content)
{
	Ptr<Label> label = *new Label();

	if (name != "")
	{
		label->SetName(name.c_str());
		RegisterName(name, label);
	}

	if (content != "")
		label->SetContent(content.c_str());

	Style* style = FrameworkElement::FindResource<Style>(styleKey.c_str());
	label->SetStyle(style);
	parent->GetChildren()->Add(label);
}

void LobbyGUI::RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* comp)
{
	FrameworkElement::GetView()->GetContent()->RegisterName(name.c_str(), comp);
}
