#include "Game/ECS/Systems/CameraSystem.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Log/Log.h"

#include "Rendering/LineRenderer.h"

#include "Engine/EngineConfig.h"

#include "Application/API/Events/EventQueue.h"

namespace LambdaEngine
{
	CameraSystem CameraSystem::s_Instance;

	CameraSystem::~CameraSystem()
	{
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &CameraSystem::OnKeyPressed);
	}

	bool CameraSystem::Init()
	{
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_CameraEntities,
					.ComponentAccesses =
					{
						{RW, CameraComponent::Type()},
						{RW, ViewProjectionMatricesComponent::Type()},
						{RW, VelocityComponent::Type()},
						{NDA, PositionComponent::Type()},
						{RW, RotationComponent::Type()},
					},
				},
				{
					.pSubscriber = &m_AttachedCameraEntities,
					.ComponentAccesses =
					{
						{RW, CameraComponent::Type()},
						{RW, ViewProjectionMatricesComponent::Type()},
						{R, ParentComponent::Type()},
						{R, OffsetComponent::Type()},
						{RW, PositionComponent::Type()},
						{RW, RotationComponent::Type()},
						{R, StepParentComponent::Type()},
					},
				}
			};
			systemReg.SubscriberRegistration.AdditionalAccesses = { {{R, FreeCameraComponent::Type()}, {R, FPSControllerComponent::Type()}} };
			systemReg.Phase = 0;

			RegisterSystem(TYPE_NAME(CameraSystem), systemReg);
		}

		m_MainFOV = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV);

		EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &CameraSystem::OnKeyPressed);

		return true;
	}

	void CameraSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<CameraComponent>*					pCameraComponents			= pECSCore->GetComponentArray<CameraComponent>();
		ComponentArray<ViewProjectionMatricesComponent>*	pViewProjectionComponent	= pECSCore->GetComponentArray<ViewProjectionMatricesComponent>();
		const ComponentArray<ParentComponent>*				pParentComponents			= pECSCore->GetComponentArray<ParentComponent>();
		const ComponentArray<StepParentComponent>*			pStepParentComponents		= pECSCore->GetComponentArray<StepParentComponent>();
		const ComponentArray<OffsetComponent>*				pOffsetComponents			= pECSCore->GetComponentArray<OffsetComponent>();
		ComponentArray<PositionComponent>*					pPositionComponents			= pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>*					pRotationComponents			= pECSCore->GetComponentArray<RotationComponent>();

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		float32 windowWidth = float32(window->GetWidth());
		float32 windowHeight = float32(window->GetHeight());

		for (Entity entity : m_AttachedCameraEntities)
		{
			const ParentComponent&		parentComp			= pParentComponents->GetConstData(entity);
			const StepParentComponent&	stepParentComp		= pStepParentComponents->GetConstData(entity);
			CameraComponent&			cameraComp			= pCameraComponents->GetData(entity);

			RotationComponent& cameraRotationComp = pRotationComponents->GetData(entity);

			if (cameraComp.FOV != m_MainFOV)
			{
				ViewProjectionMatricesComponent& viewProjectionComponent = pViewProjectionComponent->GetData(entity);

				cameraComp.FOV = m_MainFOV;
				viewProjectionComponent.Projection = glm::perspective(
					glm::radians(cameraComp.FOV),
					windowWidth / windowHeight,
					cameraComp.NearPlane,
					cameraComp.FarPlane);
			}

			if (parentComp.Attached)
			{
				PositionComponent parentPositionComp;
				if (pPositionComponents->GetConstIf(parentComp.Parent, parentPositionComp))
				{
					const OffsetComponent& cameraOffsetComp		= pOffsetComponents->GetConstData(entity);
					PositionComponent& cameraPositionComp		= pPositionComponents->GetData(entity);
					cameraPositionComp.Position					= parentPositionComp.Position + cameraOffsetComp.Offset;
				}		

				RotationComponent parentRotationComp;
				if (pRotationComponents->GetConstIf(stepParentComp.Owner, parentRotationComp))
				{
					cameraRotationComp.Quaternion			= parentRotationComp.Quaternion;
				}
			}

			if (cameraComp.ScreenShakeTime > 0.0f)
			{
				cameraComp.ScreenShakeAngle += glm::half_pi<float32>();
				cameraRotationComp.Quaternion *=
					glm::quat(
						glm::vec3(
							glm::cos(cameraComp.ScreenShakeAngle) * cameraComp.ScreenShakeTime,
							glm::sin(cameraComp.ScreenShakeAngle) * cameraComp.ScreenShakeTime,
							0.0f) * cameraComp.ScreenShakeAmplitude);
				cameraComp.ScreenShakeTime -= float32(deltaTime.AsSeconds());
			}
		}
	}

	void CameraSystem::MainThreadTick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<CameraComponent>* pCameraComponents = pECSCore->GetComponentArray<CameraComponent>();
		ComponentArray<ViewProjectionMatricesComponent>* pViewProjectionComponent = pECSCore->GetComponentArray<ViewProjectionMatricesComponent>();
		ComponentArray<FreeCameraComponent>* pFreeCameraComponents = pECSCore->GetComponentArray<FreeCameraComponent>();
		const ComponentArray<FPSControllerComponent>* pFPSCameraComponents = pECSCore->GetComponentArray<FPSControllerComponent>();
		ComponentArray<RotationComponent>* pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECSCore->GetComponentArray<VelocityComponent>();

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		float32 windowWidth = float32(window->GetWidth());
		float32 windowHeight = float32(window->GetHeight());

		for (Entity entity : m_CameraEntities)
		{
			auto& cameraComp = pCameraComponents->GetData(entity);
			if (cameraComp.FOV != m_MainFOV)
			{
				ViewProjectionMatricesComponent& viewProjectionComponent = pViewProjectionComponent->GetData(entity);

				cameraComp.FOV = m_MainFOV;
				viewProjectionComponent.Projection = glm::perspective(
					glm::radians(cameraComp.FOV),
					windowWidth / windowHeight,
					cameraComp.NearPlane,
					cameraComp.FarPlane);
			}

			if (cameraComp.IsActive)
			{
				auto& rotationComp = pRotationComponents->GetData(entity);
				auto& velocityComp = pVelocityComponents->GetData(entity);

				if (pFreeCameraComponents != nullptr && pFreeCameraComponents->HasComponent(entity))
				{
					MoveFreeCamera(dt, velocityComp, rotationComp, pFreeCameraComponents->GetConstData(entity));
				}
				else if (pFPSCameraComponents && pFPSCameraComponents->HasComponent(entity))
				{
					MoveFPSCamera(dt, velocityComp, rotationComp, pFPSCameraComponents->GetConstData(entity));
				}

/*#ifdef LAMBDA_DEBUG
				if (Input::IsKeyDown(EInputLayer::GAME, EKey::KEY_T))
				{
					RenderFrustum(entity, pPositionComponents->GetData(entity), rotationComp);
				}
#endif // LAMBDA_DEBUG*/
			}
		}
	}

	void CameraSystem::MoveFreeCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FreeCameraComponent& freeCamComp)
	{
		glm::vec3& velocity = velocityComp.Velocity;
		velocity = {
			float(InputActionSystem::IsActive(EAction::ACTION_MOVE_RIGHT)		- InputActionSystem::IsActive(EAction::ACTION_MOVE_LEFT)),		// X: Right
			float(InputActionSystem::IsActive(EAction::ACTION_CAM_UP)			- InputActionSystem::IsActive(EAction::ACTION_CAM_DOWN)),		// Y: Up
			float(InputActionSystem::IsActive(EAction::ACTION_MOVE_FORWARD)		- InputActionSystem::IsActive(EAction::ACTION_MOVE_BACKWARD))	// Z: Forward
		};

		const glm::vec3 forward = GetForward(rotationComp.Quaternion);

		if (glm::length2(velocity) > glm::epsilon<float>())
		{
			const glm::vec3 right = GetRight(rotationComp.Quaternion);
			const float shiftSpeedFactor = InputActionSystem::IsActive(EAction::ACTION_MOVE_SPRINT) ? 2.0f : 1.0f;
			velocity = glm::normalize(velocity) * freeCamComp.SpeedFactor * shiftSpeedFactor;

			velocity = velocity.x * right + velocity.y * GetUp(rotationComp.Quaternion) + velocity.z * forward;
		}

		RotateCamera(dt, forward, rotationComp.Quaternion);
	}

	void CameraSystem::MoveFPSCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FPSControllerComponent& FPSComp)
	{
		// First calculate translation relative to the character's rotation (i.e. right, up, forward).
		// Then convert the translation be relative to the world axes.
		glm::vec3& velocity = velocityComp.Velocity;

		glm::vec2 horizontalVelocity = {
			float(InputActionSystem::IsActive(EAction::ACTION_MOVE_RIGHT)		- InputActionSystem::IsActive(EAction::ACTION_MOVE_LEFT)),		// X: Right
			float(InputActionSystem::IsActive(EAction::ACTION_MOVE_FORWARD)	- InputActionSystem::IsActive(EAction::ACTION_MOVE_BACKWARD))	// Y: Forward
		};

		if (glm::length2(horizontalVelocity) > glm::epsilon<float>())
		{
			const int8 isSprinting = InputActionSystem::IsActive(EAction::ACTION_MOVE_SPRINT);
			const float32 sprintFactor = std::max(1.0f, FPSComp.SprintSpeedFactor * isSprinting);
			horizontalVelocity = glm::normalize(horizontalVelocity) * FPSComp.SpeedFactor * sprintFactor;
		}

		velocity.x = horizontalVelocity.x;
		velocity.z = horizontalVelocity.y;

		const glm::vec3 forward	= GetForward(rotationComp.Quaternion);
		const glm::vec3 right	= GetRight(rotationComp.Quaternion);

		const glm::vec3 forwardHorizontal	= glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
		const glm::vec3 rightHorizontal		= glm::normalize(glm::vec3(right.x, 0.0f, right.z));

		velocity = velocity.x * rightHorizontal + velocity.y * g_DefaultUp + velocity.z * forwardHorizontal;

		RotateCamera(dt, forward, rotationComp.Quaternion);
	}

	void CameraSystem::RotateCamera(float32 dt, const glm::vec3& forward, glm::quat& rotation)
	{
		UNREFERENCED_VARIABLE(dt);

		float addedPitch = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_UP) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_DOWN));
		float addedYaw = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_LEFT) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_RIGHT));

		addedPitch *= InputActionSystem::GetLookSensitivityPercentage();
		addedYaw *= InputActionSystem::GetLookSensitivityPercentage();

		const EInputLayer currentInputLayer = Input::GetCurrentInputLayer();

		if (m_MouseEnabled && (currentInputLayer == EInputLayer::GAME || currentInputLayer == EInputLayer::DEAD))
		{
			const MouseState& mouseState = Input::GetMouseState(currentInputLayer);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const int32 halfWidth = int32(0.5f * float32(window->GetWidth()));
			const int32 halfHeight = int32(0.5f * float32(window->GetHeight()));

			const glm::vec2 mouseDelta(mouseState.Position.x - halfWidth, mouseState.Position.y - halfHeight);

			if (glm::length(mouseDelta) > glm::epsilon<float>())
			{
				addedYaw -= InputActionSystem::GetLookSensitivity() * (float)mouseDelta.x;
				addedPitch -= InputActionSystem::GetLookSensitivity() * (float)mouseDelta.y;
			}

			CommonApplication::Get()->SetMousePosition(halfWidth, halfHeight);
		}

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		const float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		const float currentYaw = GetYaw(forward) + addedYaw;

		rotation =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
	}

	void CameraSystem::RenderFrustum(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp)
	{
		UNREFERENCED_VARIABLE(positionComp);
		UNREFERENCED_VARIABLE(rotationComp);

		LineRenderer* pLineRenderer = LineRenderer::Get();

		if (pLineRenderer != nullptr)
		{
			// This is a test code - This should probably not be done every tick
			ECSCore* pECSCore = ECSCore::GetInstance();
			auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
			auto& rotComp = pECSCore->GetComponent<RotationComponent>(entity);
			auto& camComp = pECSCore->GetComponent<CameraComponent>(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const float aspect = (float)window->GetWidth() / (float)window->GetHeight();
			const float tang = tan(glm::radians(camComp.FOV / 2));
			const float nearHeight = camComp.NearPlane * tang;
			const float nearWidth = nearHeight * aspect;
			const float farHeight = camComp.FarPlane * tang;
			const float farWidth = farHeight * aspect;

			const glm::vec3 forward = GetForward(rotComp.Quaternion);
			const glm::vec3 right = GetRight(rotComp.Quaternion);
			const glm::vec3 up = GetUp(rotComp.Quaternion);

			TArray<glm::vec3> points(10);
			const glm::vec3 nearPos = posComp.Position + forward * camComp.NearPlane;
			const glm::vec3 farPos = posComp.Position + forward * camComp.FarPlane;
			// Near TL -> Far TL
			points[0] = nearPos - right * nearWidth + up * nearHeight;
			points[1] = farPos - right * farWidth + up * farHeight;

			// Near BL -> Far BL
			points[2] = nearPos - right * nearWidth - up * nearHeight;
			points[3] = farPos - right * farWidth - up * farWidth;

			// Near TR -> Far TR
			points[4] = nearPos + right * nearWidth + up * nearHeight;
			points[5] = farPos + right * farWidth + up * farHeight;

			// Near BR -> Far BR
			points[6] = nearPos + right * nearWidth - up * nearHeight;
			points[7] = farPos + right * farWidth - up * farHeight;

			// Far TL -> Far TR
			points[8] = farPos - right * farWidth + up * farHeight;
			points[9] = farPos + right * farWidth + up * farHeight;

			if (m_LineGroupEntityIDs.contains(entity))
			{
				m_LineGroupEntityIDs[entity] = pLineRenderer->UpdateLineGroup(m_LineGroupEntityIDs[entity], points, { 0.0f, 1.0f, 0.0f });
			}
			else
			{
				m_LineGroupEntityIDs[entity] = pLineRenderer->UpdateLineGroup(UINT32_MAX, points, { 0.0f, 1.0f, 0.0f });
			}
		}
	}

	bool CameraSystem::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.Key == EKey::KEY_KEYPAD_0 || event.Key == EKey::KEY_END)
		{
			m_MouseEnabled = !m_MouseEnabled;
			CommonApplication::Get()->SetMouseVisibility(!m_MouseEnabled);
		}

		return false;
	}
}
