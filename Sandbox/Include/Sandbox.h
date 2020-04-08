#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class ResourceManager;
	class AudioListener;
	class SoundEffect3D;
	class SoundInstance3D;
	class AudioGeometry;
	class ReverbSphere;

	class Scene;
	class GameObject;
}

class Sandbox : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IMouseHandler
{
public:
	Sandbox();
	~Sandbox();

	void InitTestAudio();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp dt) override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

	// Inherited via IMouseHandler
	virtual void OnMouseMove(int32 x, int32 y)                          override;
	virtual void OnButtonPressed(LambdaEngine::EMouseButton button)     override;
	virtual void OnButtonReleased(LambdaEngine::EMouseButton button)    override;
	virtual void OnScroll(int32 delta)                                  override;

private:
	LambdaEngine::ResourceManager*			m_pResourceManager;

	GUID_Lambda								m_ToneSoundEffectGUID;
	LambdaEngine::SoundEffect3D*			m_pToneSoundEffect;
	LambdaEngine::SoundInstance3D*			m_pToneSoundInstance;

	GUID_Lambda								m_GunSoundEffectGUID;
	LambdaEngine::SoundEffect3D*			m_pGunSoundEffect;

	bool									m_SpawnPlayAts;
	float									m_GunshotTimer;
	float									m_GunshotDelay;
	float									m_Timer;

	LambdaEngine::AudioListener*			m_pAudioListener;

	LambdaEngine::ReverbSphere*				m_pReverbSphere;
	LambdaEngine::AudioGeometry*			m_pAudioGeometry;

	LambdaEngine::Scene*					m_pScene;
};
