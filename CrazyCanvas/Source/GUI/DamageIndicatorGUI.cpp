#include "GUI/DamageIndicatorGUI.h"

#include "NoesisPCH.h"


DamageIndicatorGUI::DamageIndicatorGUI()
{
	Noesis::GUI::LoadComponent(this, "DamageIndicatorGUI.xaml");

	m_pDamageIndicatorStoryboard	= FindResource<Noesis::Storyboard>("DamageIndicatorStoryboard");
	m_pIndicatorImage				= FindName<Noesis::Image>("DamageIndicator");
}

DamageIndicatorGUI::~DamageIndicatorGUI()
{
}

bool DamageIndicatorGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void DamageIndicatorGUI::DisplayIndicator()
{
	m_pDamageIndicatorStoryboard->Begin();
	//m_pDamageIndicatorStoryboard->Stop();
}