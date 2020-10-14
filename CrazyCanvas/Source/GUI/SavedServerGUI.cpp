#pragma once 

#include "GUI/SavedServerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"


using namespace LambdaEngine;
using namespace Noesis;


SavedServerGUI::SavedServerGUI(const LambdaEngine::String& xamlFile) :
	m_ItemCount(0)
{

}

void SavedServerGUI::Init(ListBox* pSavedListView, ListBox* pLocalListView)
{
	m_SavedServerList = *pSavedListView;
	m_LocalServerList = *pLocalListView;
}

SavedServerGUI::~SavedServerGUI()
{
}

Ptr<Grid> SavedServerGUI::AddSavedServerItem(Grid* pParentGrid, const ServerInfo& serverInfo, bool isRunning)
{
	Ptr<Grid> grid = AddServerItem(pParentGrid, serverInfo, isRunning);
	if (m_SavedServerList->GetItems()->Add(grid) == -1)
	{
		LOG_ERROR("Refreshing Saved List");
	}

	return grid;
}

Ptr<Grid> SavedServerGUI::AddLocalServerItem(Grid* pParentGrid, const ServerInfo& serverInfo, bool isRunning)
{
	Ptr<Grid> grid = AddServerItem(pParentGrid, serverInfo, isRunning);
	if (m_LocalServerList->GetItems()->Add(grid) == -1)
	{
		LOG_ERROR("Refreshing Local List");
	}

	return grid;
}

Ptr<Grid> SavedServerGUI::AddServerItem(Grid* pParentGrid, const ServerInfo& serverInfo, bool isRunning)
{
	Ptr<Grid> grid = *new Grid();

	for (int i = 0; i < 5; i++)
	{
		Ptr<ColumnDefinition> columnDef = *new ColumnDefinition();
		GridLength gridLength = GridLength(20, GridUnitType_Star);
		columnDef->SetWidth(gridLength);
		grid->GetColumnDefinitions()->Add(columnDef);
	}

	//m_pSavedServerList->SetItemsSource(Servers);
	//m_pSavedServerList->GetChildren()->Add(m_pSavedServerList);
	//gl�m inte s�tta row ocks� n�r du l�st detta bajsproblemet. 
	pParentGrid->SetColumn(grid, 1);
	pParentGrid->SetColumnSpan(grid, 2);
	pParentGrid->SetRow(grid, 4);

	Ptr<TextBlock> serverName = *new TextBlock();
	Ptr<TextBlock> mapName = *new TextBlock();
	Ptr<TextBlock> players = *new TextBlock();
	Ptr<TextBlock> ping = *new TextBlock();
	Ptr<Rectangle> isRun = *new Rectangle();
	Ptr<SolidColorBrush> brush = *new SolidColorBrush();


	serverName->SetText(serverInfo.Name.c_str());
	mapName->SetText(serverInfo.MapName.c_str());
	players->SetText(std::to_string(serverInfo.Players).c_str());
	ping->SetText(std::to_string(serverInfo.Ping).c_str());

	Color color = Color();

	if (isRun)
		brush->SetColor(color.Green());
	else
		brush->SetColor(color.Red());

	isRun->SetFill(brush);

	grid->GetChildren()->Add(serverName);
	grid->GetChildren()->Add(mapName);
	grid->GetChildren()->Add(players);
	grid->GetChildren()->Add(ping);
	grid->GetChildren()->Add(isRun);

	grid->SetColumn(serverName, 0);
	grid->SetColumn(mapName, 1);
	grid->SetColumn(players, 2);
	grid->SetColumn(ping, 3);
	grid->SetColumn(isRun, 4);
	//grid->SetRow(pServerName, m_ItemCount++);

	return grid;
}


void SavedServerGUI::UpdateServerItems(const ServerInfo& serverInfo)
{
	TextBlock* serverName	= (TextBlock*)serverInfo.ServerGrid->GetChildren()->Get(0);
	TextBlock* mapName		= (TextBlock*)serverInfo.ServerGrid->GetChildren()->Get(1);
	TextBlock* playerCount	= (TextBlock*)serverInfo.ServerGrid->GetChildren()->Get(2);
	TextBlock* ping			= (TextBlock*)serverInfo.ServerGrid->GetChildren()->Get(3);

	serverName->SetText(serverInfo.Name.c_str());
	mapName->SetText(serverInfo.MapName.c_str());
	ping->SetText(std::to_string(serverInfo.Ping).c_str());
	playerCount->SetText(std::to_string(serverInfo.Players).c_str());



	/*
	if (!isLocal) 
	{
		ItemCollection* pItems = m_SavedServerList->GetItems();

		Ptr<Grid> grid;

		if (pItems->Count())
		{
			for (int i = 0; i < pItems->Count(); i++)
			{
				grid = *((Grid*)pItems->GetItemAt(i).GetPtr());

				TextBlock* element = (TextBlock*)grid->GetChildren()->Get(0);
				element->SetText(pServerN);
				element = (TextBlock*)grid->GetChildren()->Get(1);
				element->SetText(pMapN);
				element = (TextBlock*)grid->GetChildren()->Get(2);
				element->SetText(pPing);

				Rectangle* rect = (Rectangle*)grid->GetChildren()->Get(3);
				Ptr<SolidColorBrush> brush = *new SolidColorBrush();
				Color color = Color();

				if (isRunning)
				{
					brush->SetColor(color.Green());
					rect->SetFill(brush);
				}
				else
				{
					brush->SetColor(color.Red());
					rect->SetFill(brush);
				}

			}
			LOG_MESSAGE(grid->GetChildren()->Get(1)->ToString().Str());
		}
	}
	else
	{
		ItemCollection* pItems = m_LocalServerList->GetItems();

		Ptr<Grid> grid = *((Grid*)pItems->GetItemAt(0).GetPtr());

		if (pItems->Count())
		{
			LOG_MESSAGE(grid->GetChildren()->Get(0)->ToString().Str());
		}
	}*/
}

