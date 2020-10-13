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

void SavedServerGUI::Init(ListBox* pListView)
{
	/*textBlock = *new ObservableCollection<TextBlock>();
	tBlock = *new TextBlock("ChrilleBoi");
	textBlock->Add(tBlock);
	m_pSavedServerList->SetItemsSource(textBlock);*/
	
	m_SavedServerList = *pListView;

}

SavedServerGUI::~SavedServerGUI()
{
}

void SavedServerGUI::AddServerItem(Grid* pParentGrid, const char* pServerN, const char* pMapN, const char* pPing, bool isRunning)
{
	
	Ptr<Grid> grid = *new Grid();

	for (int i = 0; i < 4; i++)
	{
		Ptr<ColumnDefinition> columnDef = *new ColumnDefinition();
		GridLength gridLength = GridLength(25, GridUnitType_Star);
		columnDef->SetWidth(gridLength);
		grid->GetColumnDefinitions()->Add(columnDef);
	}

	//m_pSavedServerList->SetItemsSource(Servers);
	//m_pSavedServerList->GetChildren()->Add(m_pSavedServerList);
	//gl�m inte s�tta row ocks� n�r du l�st detta bajsproblemet. 
	pParentGrid->SetColumn(grid, 1);
	pParentGrid->SetColumnSpan(grid, 2);
	pParentGrid->SetRow(grid, 4);

	Ptr<TextBlock> serverName	= *new TextBlock();
	Ptr<TextBlock> mapName		= *new TextBlock();
	Ptr<TextBlock> ping		= *new TextBlock();
	Ptr<Rectangle> isRun	= *new Rectangle();

	serverName->SetText(pServerN);
	mapName->SetText(pMapN);
	ping->SetText(pPing);

	Ptr<SolidColorBrush> brush = *new SolidColorBrush();
	Color color = Color();

	if (isRun)
		brush->SetColor(color.Green());
	else
		brush->SetColor(color.Red());

	isRun->SetFill(brush);

	grid->GetChildren()->Add(serverName);
	grid->GetChildren()->Add(mapName);
	grid->GetChildren()->Add(ping);
	grid->GetChildren()->Add(isRun);

	grid->SetColumn(serverName, 0);
	grid->SetColumn(mapName, 1);
	grid->SetColumn(ping, 2);
	grid->SetColumn(isRun, 3);
	//grid->SetRow(pServerName, m_ItemCount++);


	LOG_MESSAGE(pParentGrid->GetName());
	LOG_MESSAGE(m_SavedServerList->GetName());

	if (m_SavedServerList->GetItems()->Add(grid) == -1)
	{
		LOG_ERROR("SKIT ON ME");
	}
}

