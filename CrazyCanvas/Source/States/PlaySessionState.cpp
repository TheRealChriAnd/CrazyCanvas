#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Audio/ListenerComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Rendering/EntityMaskManager.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include "Lobby/PlayerManagerClient.h"

#include "Game/StateManager.h"
#include "States/MainMenuState.h"

#include "Teams/TeamHelper.h"

#include "GUI/GUIHelpers.h"

#include "Game/GameConsole.h"
#include "Resources/ResourceCatalog.h"

#include "ECS/Systems/Misc/DestructionSystem.h"

#include "Rendering/ImGuiRenderer.h"
#include "Debug/Profiler.h"

using namespace LambdaEngine;

PlaySessionState* PlaySessionState::s_pInstance = nullptr;

PlaySessionState::PlaySessionState(const PacketGameSettings& gameSettings, bool singlePlayer) :
	m_Singleplayer(singlePlayer),
	m_MultiplayerClient(),
	m_GameSettings(gameSettings),
	m_NameplateSystem(&m_HUDSystem),
	m_DefferedTicks(3),
	m_Initiated(false),
	m_MatchReadyReceived(false),
	m_MatchLoaded(false)
{
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Init();
	}

	// Update Team colors and materials
	TeamHelper::SetTeamColor(1, gameSettings.TeamColor1);
	TeamHelper::SetTeamColor(2, gameSettings.TeamColor2);

	// Set Team Paint colors
	auto& renderSystem = RenderSystem::GetInstance();
	renderSystem.SetPaintMaskColor(1, TeamHelper::GetTeamColor(1));
	renderSystem.SetPaintMaskColor(2, TeamHelper::GetTeamColor(2));

	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketMatchReady>>(this, &PlaySessionState::OnPacketMatchReadyReceived);
}

PlaySessionState::~PlaySessionState()
{
	s_pInstance = nullptr;
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Release();
	}

	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketMatchReady>>(this, &PlaySessionState::OnPacketMatchReadyReceived);

	Match::Release();
}

void PlaySessionState::Init()
{
	s_pInstance = this;

	if (!m_Singleplayer)
	{
		//Called to tell the server we are ready to load the match
		PlayerManagerClient::SetLocalPlayerStateLoading();
		m_CamSystem.Init();
	}
	else
	{
		ConsoleCommand cmdWireframe;
		cmdWireframe.Init("wireframe", false);
		cmdWireframe.AddArg(Arg::EType::BOOL);
		cmdWireframe.AddDescription("Activate/Deactivate wireframe mode\n");
		GameConsole::Get().BindCommand(cmdWireframe, [&, this](GameConsole::CallbackInput& input)->void 
		{
			THashTable<uint64, ManagedGraphicsPipelineStateDesc>& graphicsPipelinesDescs = PipelineStateManager::GetGraphicsPipelineStateDescriptions();

			for (auto& it : graphicsPipelinesDescs)
			{
				ManagedGraphicsPipelineStateDesc& pipelineStateDesc = it.second;
				if (pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS" || pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS_MESH_PAINT")
				{
					pipelineStateDesc.RasterizerState.PolygonMode = input.Arguments.GetFront().Value.Boolean ? EPolygonMode::POLYGON_MODE_LINE : EPolygonMode::POLYGON_MODE_FILL;
				}
			}

			PipelineStateRecompileEvent recompileEvent = {};
			EventQueue::SendEvent(recompileEvent);
		});

		ConsoleCommand cmdMeshletDbg;
		cmdMeshletDbg.Init("meshlet_debug", false);
		cmdMeshletDbg.AddArg(Arg::EType::BOOL);
		cmdMeshletDbg.AddDescription("Activate/Deactivate meshlet debug\n");
		GameConsole::Get().BindCommand(cmdMeshletDbg, [&, this](GameConsole::CallbackInput& input)->void
			{
				THashTable<uint64, ManagedGraphicsPipelineStateDesc>& graphicsPipelinesDescs = PipelineStateManager::GetGraphicsPipelineStateDescriptions();

				for (auto& it : graphicsPipelinesDescs)
				{
					ManagedGraphicsPipelineStateDesc& pipelineStateDesc = it.second;
					if (pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS" || pipelineStateDesc.DebugName == "DEFERRED_GEOMETRY_PASS_MESH_PAINT")
					{
						if (pipelineStateDesc.MeshShader.ShaderGUID != GUID_NONE)
						{
							ShaderConstant constant;
							constant.Integer = input.Arguments.GetFront().Value.Boolean ? 1 : 0;

							// Mesh shader
							if (pipelineStateDesc.MeshShader.ShaderConstants.GetSize() < 1)
							{
								pipelineStateDesc.MeshShader.ShaderConstants.EmplaceBack(constant);
							}
							else
							{
								pipelineStateDesc.MeshShader.ShaderConstants[0] = constant;
							}

							// Pixel shader
							if (pipelineStateDesc.PixelShader.ShaderConstants.GetSize() < 1)
							{
								pipelineStateDesc.PixelShader.ShaderConstants.EmplaceBack(constant);
							}
							else
							{
								pipelineStateDesc.PixelShader.ShaderConstants[0] = constant;
							}
						}
					}
				}

				PipelineStateRecompileEvent recompileEvent = {};
				EventQueue::SendEvent(recompileEvent);
			});
	}
  
	ConsoleCommand cmdDebugWindow;
	cmdDebugWindow.Init("show_debug_window", false);
	cmdDebugWindow.AddArg(Arg::EType::BOOL);
	cmdDebugWindow.AddDescription("Activate/Deactivate debugging window.\n\t'show_debug_window true'");
	GameConsole::Get().BindCommand(cmdDebugWindow, [&, this](GameConsole::CallbackInput& input)->void {
		m_DebugWindow = input.Arguments.GetFront().Value.Boolean;
		});

	CommonApplication::Get()->SetMouseVisibility(false);
	PlayerActionSystem::SetMouseEnabled(true);

	const EInputLayer currentInputLayer = Input::GetCurrentInputLayer();
	if (currentInputLayer == EInputLayer::DEBUG)
	{
		Input::PopInputLayer();
		Input::PushInputLayer(EInputLayer::GAME);
		Input::PushInputLayer(EInputLayer::DEBUG);
	}
	else
		Input::PushInputLayer(EInputLayer::GAME);

	EnablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Pause();

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_PingHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Init Systems
	m_HUDSystem.Init();
	m_NameplateSystem.Init();
	m_DestructionSystem.Init();

	// Commands
	ConsoleCommand cmd1;
	cmd1.Init("update_shaders", true);
	cmd1.AddDescription("Update all shaders. \n\t'update_shaders'");
	GameConsole::Get().BindCommand(cmd1, [&, this](GameConsole::CallbackInput& input)->void {
		m_UpdateShaders = true;
	});
}

void PlaySessionState::InternalInit()
{
	// Load Match
	const TArray<SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

	MatchDescription matchDescription =
	{
		.LevelHash = levelHashes[m_GameSettings.MapID],
		.GameMode = m_GameSettings.GameMode,
		.MaxScore = m_GameSettings.FlagsToWin,
	};
	Match::CreateMatch(&matchDescription);

	m_MatchLoaded = true;
	TryFinishMatchLoading();

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Setup();
	}
}

void PlaySessionState::TryFinishMatchLoading()
{
	if(m_MatchLoaded && m_MatchReadyReceived)
		PlayerManagerClient::SetLocalPlayerStateLoaded();
}

void PlaySessionState::Tick(Timestamp delta)
{
	if (m_UpdateShaders)
	{
		m_UpdateShaders = false;
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	if (m_DebugWindow)
	{
		Profiler::Tick(delta);
		Profiler::Render();
	}

	m_MultiplayerClient.TickMainThreadInternal(delta);
}

void PlaySessionState::FixedTick(Timestamp delta)
{
	PROFILE_FUNCTION("m_NameplateSystem.FixedTick", m_NameplateSystem.FixedTick(delta));
	PROFILE_FUNCTION("m_HUDSystem.FixedTick", m_HUDSystem.FixedTick(delta));
	PROFILE_FUNCTION("MultiplayerBase::FixedTickMainThreadInternal", m_MultiplayerClient.FixedTickMainThreadInternal(delta));

	if (!m_Initiated)
	{
		if (s_pInstance)
		{
			if (--m_DefferedTicks == 0)
			{
				InternalInit();
				m_Initiated = true;
			}
		}
	}
}

bool PlaySessionState::OnClientDisconnected(const ClientDisconnectedEvent& event)
{
	const String& reason = event.Reason;

	LOG_WARNING("PlaySessionState::OnClientDisconnected(Reason: %s)", reason.c_str());

	PlayerManagerClient::Reset();

	LambdaEngine::GUIApplication::SetView(nullptr);

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}

bool PlaySessionState::OnPacketMatchReadyReceived(const PacketReceivedEvent<PacketMatchReady>& event)
{
	UNREFERENCED_VARIABLE(event);
	m_MatchReadyReceived = true;
	TryFinishMatchLoading();
	return true;
}

const PacketGameSettings& PlaySessionState::GetGameSettings() const
{
	return m_GameSettings;
}

PlaySessionState* PlaySessionState::GetInstance()
{
	return s_pInstance;
}