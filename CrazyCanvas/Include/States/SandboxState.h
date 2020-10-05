#pragma once

#include "Game/State.h"

#include "Containers/TArray.h"

#include "ECS/ECSCore.h"
#include "ECS/Entity.h"

#include "GUI/GUITest.h"

#include "Rendering/IRenderGraphCreateHandler.h"

#include "Application/API/Events/KeyEvents.h"

#include <NsCore/Ptr.h>
#include <NsGui/IView.h>

namespace LambdaEngine
{
	class RenderGraphEditor;
}

class SandboxState : public LambdaEngine::State, public LambdaEngine::IRenderGraphCreateHandler
{
public:
	SandboxState() = default;
	~SandboxState();

	void Init() override final;

	void Resume() override final;
	void Pause() override final;

	void Tick(LambdaEngine::Timestamp delta) override final;

	void OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph) override final;

	void RenderImgui();

private:
	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[3];

	Noesis::Ptr<GUITest> m_GUITest;
	Noesis::Ptr<Noesis::IView> m_View;

	LambdaEngine::RenderGraphEditor*	m_pRenderGraphEditor	= nullptr;
	bool								m_RenderGraphWindow		= false;
	bool								m_ShowDemoWindow		= false;
	bool								m_DebuggingWindow		= false;

	bool					m_ShowTextureDebuggingWindow	= false;
	LambdaEngine::String	m_TextureDebuggingName			= "";
	GUID_Lambda				m_TextureDebuggingShaderGUID	= GUID_NONE;
};
