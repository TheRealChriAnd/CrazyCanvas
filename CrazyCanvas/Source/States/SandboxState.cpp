#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Debug/Profiler.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineConfig.h"

#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"
#include "Game/GameConsole.h"

#include "Input/API/Input.h"

#include "Math/Random.h"


#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphEditor.h"

#include "Math/Random.h"

#include "GUI/GUITest.h"

#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include <imgui.h>

#include "World/LevelManager.h"
#include "World/Level.h"

using namespace LambdaEngine;

SandboxState::~SandboxState()
{
    EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &SandboxState::OnKeyPressed));

	if (m_GUITest.GetPtr() != nullptr)
	{
		m_GUITest.Reset();
		m_View.Reset();
	}

	SAFEDELETE(m_pRenderGraphEditor);
	SAFEDELETE(m_pLevel);
}

void SandboxState::Init()
{
	// Create Systems
	TrackSystem::GetInstance().Init();
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);
	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	m_RenderGraphWindow = EngineConfig::GetBoolProperty("ShowRenderGraph");
	m_ShowDemoWindow = EngineConfig::GetBoolProperty("ShowDemo");
	m_DebuggingWindow = EngineConfig::GetBoolProperty("Debugging");

	m_GUITest	= *new GUITest("Test.xaml");
	m_View		= Noesis::GUI::CreateView(m_GUITest);
	LambdaEngine::GUIApplication::SetView(m_View);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		const CameraDesc cameraDesc =
		{
			.Position	= { 0.0f, 2.0f, 5.0f },
			.FOVDegrees	= EngineConfig::GetFloatProperty("CameraFOV"),
			.Width		= (float32)window->GetWidth(),
			.Height		= (float32)window->GetHeight(),
			.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane"),
			.FarPlane	= EngineConfig::GetFloatProperty("CameraFarPlane")
		};
		CreateFPSCameraEntity(cameraDesc);
	}

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
	}

	// Robot
	{
		TArray<GUID_Lambda> animations;
		const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		materialProperties.Roughness = 1.0f;
		materialProperties.Metallic = 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID = robotGUID;
		robotMeshComp.MaterialGUID = robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.AnimationGUID = animations[0];
		robotAnimationComp.PlaybackSpeed = 1.0f;
		robotAnimationComp.IsLooping = false;
		// TODO: Safer way than getting the raw pointer (GUID for skeletons?)
		robotAnimationComp.Pose.pSkeleton = ResourceManager::GetMesh(robotGUID)->pSkeleton;

		glm::vec3 position = glm::vec3(0.0f, 1.25f, -5.0f);
		glm::vec3 scale(0.01f);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_0", 512, 512));

		position = glm::vec3(0.0f, 1.25f, 0.0f);
		robotAnimationComp.IsLooping = true;
		robotAnimationComp.NumLoops = 10;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_1", 512, 512));

		position = glm::vec3(-5.0f, 1.25f, 0.0f);
		robotAnimationComp.NumLoops = INFINITE_LOOPS;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_2", 512, 512));

		position = glm::vec3(5.0f, 1.25f, 0.0f);

		robotAnimationComp.PlaybackSpeed *= -1.0f;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_3", 512, 512));

		// Audio
		GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("halo_theme.wav");
		ISoundInstance3D* pSoundInstance = new SoundInstance3DFMOD(AudioAPI::GetDevice());
		const SoundInstance3DDesc desc =
		{
			.pName = "RobotSoundInstance",
			.pSoundEffect = ResourceManager::GetSoundEffect(soundGUID),
			.Flags = FSoundModeFlags::SOUND_MODE_NONE,
			.Position = position,
			.Volume = 0.03f
		};

		pSoundInstance->Init(&desc);
		pECS->AddComponent<AudibleComponent>(entity, { pSoundInstance });
	}

	// Emitter
	{
		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, {-2.0f, 4.0f, 0.0f } });
		pECS->AddComponent<RotationComponent>(entity, { true,glm::identity<glm::quat>() });
		pECS->AddComponent<ParticleEmitterComponent>(entity, ParticleEmitterComponent{ .Velocity = 1.0f, .Acceleration = 0.0f, .ParticleRadius = 0.1f });
	}

	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

		uint32 gridRadius = 5;

		for (uint32 y = 0; y < gridRadius; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness = roughness;
				materialProperties.Metallic = metallic;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				glm::vec3 position(-float32(gridRadius) * 0.5f + x, 2.0f + y, 4.0f);
				glm::vec3 scale(1.0f);

				Entity entity = pECS->CreateEntity();
				const StaticCollisionInfo collisionCreateInfo = {
					.Entity = entity,
					.Position = pECS->AddComponent<PositionComponent>(entity, { true, position }),
					.Scale = pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
					.Rotation = pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
					.Mesh = pECS->AddComponent<MeshComponent>(entity, sphereMeshComp),
					.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
					.CollisionMask = ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
				};

				pPhysicsSystem->CreateCollisionSphere(collisionCreateInfo);
			}
		}
	}

	if constexpr (IMGUI_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());

		m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();
		m_pRenderGraphEditor->InitGUI();	//Must Be called after Renderer is initialized
	}

	ConsoleCommand cmd1;
	cmd1.Init("render_graph", true);
	cmd1.AddArg(Arg::EType::BOOL);
	cmd1.AddDescription("Activate/Deactivate rendergraph window.\n\t'render_graph true'");
	GameConsole::Get().BindCommand(cmd1, [&, this](GameConsole::CallbackInput& input)->void {
		m_RenderGraphWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd2;
	cmd2.Init("imgui_demo", true);
	cmd2.AddArg(Arg::EType::BOOL);
	cmd2.AddDescription("Activate/Deactivate demo window.\n\t'imgui_demo true'");
	GameConsole::Get().BindCommand(cmd2, [&, this](GameConsole::CallbackInput& input)->void {
		m_ShowDemoWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd3;
	cmd3.Init("show_debug_window", false);
	cmd3.AddArg(Arg::EType::BOOL);
	cmd3.AddDescription("Activate/Deactivate debugging window.\n\t'show_debug_window true'");
	GameConsole::Get().BindCommand(cmd3, [&, this](GameConsole::CallbackInput& input)->void {
		m_DebuggingWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand showTextureCMD;
	showTextureCMD.Init("debug_texture", true);
	showTextureCMD.AddArg(Arg::EType::BOOL);
	showTextureCMD.AddFlag("t", Arg::EType::STRING, 6);
	showTextureCMD.AddFlag("ps", Arg::EType::STRING);
	showTextureCMD.AddDescription("Show a texture resource which is used in the RenderGraph.\n\t'Example: debug_texture 1 -t TEXTURE_NAME1 TEXTURE_NAME2 ...'", { {"t", "The textures you want to display. (separated by spaces)"}, {"ps", "Which pixel shader you want to use."} });
	GameConsole::Get().BindCommand(showTextureCMD, [&, this](GameConsole::CallbackInput& input)->void
		{
			m_ShowTextureDebuggingWindow = input.Arguments.GetFront().Value.Boolean;

			auto textureNameIt = input.Flags.find("t");
			auto shaderNameIt = input.Flags.find("ps");

			GUID_Lambda textureDebuggingShaderGUID = shaderNameIt != input.Flags.end() ? ResourceManager::GetShaderGUID(shaderNameIt->second.Arg.Value.String) : GUID_NONE;
			if (textureNameIt != input.Flags.end())
			{
				m_TextureDebuggingNames.Resize(textureNameIt->second.NumUsedArgs);
				for (uint32 i = 0; i < textureNameIt->second.NumUsedArgs; i++)
				{
					m_TextureDebuggingNames[i].ResourceName = textureNameIt->second.Args[i].Value.String;
					m_TextureDebuggingNames[i].PixelShaderGUID = textureDebuggingShaderGUID;
				}
			}
		});
}

void SandboxState::Resume()
{
	// Unpause System

	// Reload Page
}

void SandboxState::Pause()
{
	// Pause System

	// Unload Page
}

void SandboxState::Tick(LambdaEngine::Timestamp delta)
{
	// Update State specfic objects
	m_pRenderGraphEditor->Update();
	LambdaEngine::Profiler::Tick(delta);

	if constexpr (IMGUI_ENABLED)
	{
		RenderImgui();
	}
}

void SandboxState::OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph)
{
	using namespace LambdaEngine;

	Sampler* pNearestSampler				= Sampler::GetNearestSampler();

	GUID_Lambda blueNoiseID = ResourceManager::GetTextureGUID("Blue Noise Texture");

	Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
	TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);

	ResourceUpdateDesc blueNoiseUpdateDesc = {};
	blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pNearestSampler;

	pRenderGraph->UpdateResource(&blueNoiseUpdateDesc);

	GUID_Lambda cubemapTexID = ResourceManager::GetTextureGUID("Cubemap Texture");

	Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
	TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);

	ResourceUpdateDesc cubeTextureUpdateDesc = {};
	cubeTextureUpdateDesc.ResourceName = "SKYBOX";
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

	pRenderGraph->UpdateResource(&cubeTextureUpdateDesc);
}

void SandboxState::RenderImgui()
{
	using namespace LambdaEngine;

	ImGuiRenderer::Get().DrawUI([&]()
	{
		if (m_RenderGraphWindow)
			m_pRenderGraphEditor->RenderGUI();

		if (m_ShowDemoWindow)
			ImGui::ShowDemoWindow();

		if (m_DebuggingWindow)
		{
			Profiler::Render();
		}

		if (m_ShowTextureDebuggingWindow)
		{
			if (ImGui::Begin("Texture Debugging"))
			{
				for (ImGuiTexture& imGuiTexture : m_TextureDebuggingNames)
				{
					ImGui::Image(&imGuiTexture, ImGui::GetWindowSize());
				}
			}

			ImGui::End();
		}
	});
}

bool SandboxState::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
{
	using namespace LambdaEngine;

	if (!IsEventOfType<KeyPressedEvent>(event))
	{
		return false;
	}

	if (event.IsRepeat)
	{
		return false;
	}

	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (event.Key == EKey::KEY_5)
	{
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	// Debugging Lights
	static uint32 indexLights = 0;
	static bool removeLights = true;
	ECSCore* ecsCore = ECSCore::GetInstance();
	if (event.Key == EKey::KEY_9)
	{
		uint32 modIndex = indexLights % 10U;
		if (modIndex == 0U)
			removeLights = !removeLights;

		if (!removeLights)
		{
			Entity e = ecsCore->CreateEntity();
			m_PointLights[modIndex] = e;

			ecsCore->AddComponent<PositionComponent>(e, { true, {0.0f, 2.0f, -5.0f + modIndex} });
			ecsCore->AddComponent<PointLightComponent>(e, PointLightComponent{ .ColorIntensity = {modIndex % 2U, modIndex % 3U, modIndex % 5U, 25.0f} });
		}
		else
		{
			ecsCore->RemoveEntity(m_PointLights[modIndex]);
		}

		indexLights++;
	}

	// Debugging Emitters
	static uint32 indexEmitters = 0;
	static bool removeEmitters = true;
	if (event.Key == EKey::KEY_8)
	{
		uint32 modIndex = indexEmitters % 10U;
		if (modIndex == 0U)
			removeEmitters = !removeEmitters;

		if (!removeEmitters)
		{
			Entity e = ecsCore->CreateEntity();
			m_Emitters[modIndex] = e;

			ecsCore->AddComponent<PositionComponent>(e, { true, {0.0f, 4.0f, -5.f + float(modIndex) } });
			ecsCore->AddComponent<RotationComponent>(e, { true,glm::identity<glm::quat>() });
			ecsCore->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{ .Velocity = float(modIndex), .Acceleration = 0.0f, .ParticleRadius = 0.05f * float(modIndex) });
		}
		else
		{
			ecsCore->RemoveEntity(m_PointLights[modIndex]);
		}

		indexEmitters++;
	}

	return true;
}
