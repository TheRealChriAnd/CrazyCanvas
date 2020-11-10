#include "CrazyCanvas.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Resources/ResourceManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "RenderStages/PlayerRenderer.h"
#include "States/BenchmarkState.h"
#include "States/MainMenuState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/ServerState.h"

#include "Networking/API/NetworkUtils.h"

#include "World/LevelManager.h"

#include "Teams/TeamHelper.h"

#include "Multiplayer/ServerHostHelper.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Engine/EngineConfig.h"

#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/GameOverGUI.h"
#include "GUI/HUDGUI.h"
#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"

#include "Rendering/EntityMaskManager.h"

#include "ECS/Components/Player/WeaponComponent.h"

#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>

CrazyCanvas::CrazyCanvas(const argh::parser& flagParser)
{
	using namespace LambdaEngine;

	constexpr const char* pGameName = "Crazy Canvas";
	constexpr const char* pDefaultStateStr = "crazycanvas";
	constexpr const char* pDefaultIsHostStr = "";
	State* pStartingState = nullptr;
	String stateStr;

	String AuthenticationIDStr; // Used on server To Identify Host(Client transmits HostID)
	String clientHostIDStr; // Used on Client To Identify Host(Server transmits HostID)

	flagParser({ "--state" }, pDefaultStateStr) >> stateStr;

	if (stateStr == "server")
	{
		flagParser(1, pDefaultIsHostStr) >> clientHostIDStr;

		flagParser(2, pDefaultIsHostStr) >> AuthenticationIDStr;
	}

	if (stateStr == "crazycanvas" || stateStr == "sandbox" || stateStr == "client" || stateStr == "benchmark")
	{
		ClientSystem::Init(pGameName);
	}
	else if (stateStr == "server")
	{
		ServerSystem::Init(pGameName);
	}

	if (!RegisterGUIComponents())
	{
		LOG_ERROR("Failed to Register GUI Components");
	}

	ServerHostHelper::Init();

	if (!BindComponentTypeMasks())
	{
		LOG_ERROR("Failed to bind Component Type Masks");
	}

	if (!LevelManager::Init())
	{
		LOG_ERROR("Level Manager Init Failed");
	}

	if (!TeamHelper::Init())
	{
		LOG_ERROR("Team Helper Init Failed");
	}

	RenderSystem::GetInstance().AddCustomRenderer(DBG_NEW PlayerRenderer());
	RenderSystem::GetInstance().InitRenderGraphs();

	LoadRendererResources();

	if (stateStr == "crazycanvas")
	{
		pStartingState = DBG_NEW MainMenuState();
	}
	else if (stateStr == "sandbox")
	{
		pStartingState = DBG_NEW SandboxState();
	}
	else if (stateStr == "client")
	{
		uint16 port = (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
		pStartingState = DBG_NEW PlaySessionState(false, IPEndPoint(NetworkUtils::GetLocalAddress(), port));
	}
	else if (stateStr == "server")
	{
		pStartingState = DBG_NEW ServerState(clientHostIDStr, AuthenticationIDStr);
	}
	else if (stateStr == "benchmark")
	{
		pStartingState = DBG_NEW BenchmarkState();
	}

	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::PUSH);
}

CrazyCanvas::~CrazyCanvas()
{
	if (!LevelManager::Release())
	{
		LOG_ERROR("Level Manager Release Failed");
	}
}

void CrazyCanvas::Tick(LambdaEngine::Timestamp delta)
{
	Render(delta);
}

void CrazyCanvas::FixedTick(LambdaEngine::Timestamp)
{}

void CrazyCanvas::Render(LambdaEngine::Timestamp)
{}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		return DBG_NEW CrazyCanvas(flagParser);
	}
}

bool CrazyCanvas::RegisterGUIComponents()
{
	Noesis::RegisterComponent<CountdownGUI>();
	Noesis::RegisterComponent<GameOverGUI>();
	Noesis::RegisterComponent<DamageIndicatorGUI>();
	Noesis::RegisterComponent<EnemyHitIndicatorGUI>();
	Noesis::RegisterComponent<HUDGUI>();
	Noesis::RegisterComponent<MainMenuGUI>();

	return true;
}

bool CrazyCanvas::LoadRendererResources()
{
	using namespace LambdaEngine;

	// For Skybox RenderGraph
	{
		String skybox[]
		{
			"Skybox/px.png",
			"Skybox/nx.png",
			"Skybox/py.png",
			"Skybox/ny.png",
			"Skybox/pz.png",
			"Skybox/nz.png"
		};

		GUID_Lambda cubemapTexID = ResourceManager::LoadCubeTexturesArrayFromFile("Cubemap Texture", skybox, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false, false);

		Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
		TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);
		Sampler* pNearestSampler		= Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName							= "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	// For Mesh painting in RenderGraph
	{
		GUID_Lambda brushMaskID = ResourceManager::LoadTextureFromFile("MeshPainting/BrushMaskV3.png", EFormat::FORMAT_R8G8B8A8_UNORM, false, false);

		Texture* pTexture = ResourceManager::GetTexture(brushMaskID);
		TextureView* pTextureView = ResourceManager::GetTextureView(brushMaskID);
		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "BRUSH_MASK";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	return true;
}

bool CrazyCanvas::BindComponentTypeMasks()
{
	using namespace LambdaEngine;

	EntityMaskManager::BindTypeToExtensionDesc(WeaponLocalComponent::Type(), { 0 }, false);	// Bit = 0xF

	EntityMaskManager::Finalize();

	return true;
}
