#include "Audio/AudioSystem.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	AudioDevice*	AudioSystem::s_pAudioDevice = nullptr;

	bool AudioSystem::Init()
	{
		s_pAudioDevice = new AudioDevice();

		AudioDeviceDesc audioDeviceDesc = {};
		audioDeviceDesc.pName					= "Main AudioDevice";
		audioDeviceDesc.Debug					= true;
		audioDeviceDesc.MaxNumAudioListeners	= 1;
		audioDeviceDesc.MaxWorldSize			= 200;

		if (!s_pAudioDevice->Init(audioDeviceDesc))
		{
			LOG_ERROR("[AudioSystem]: Could not initialize AudioDevice");
			return false;
		}

		return true;
	}

	bool AudioSystem::Release()
	{
		SAFEDELETE(s_pAudioDevice)
		return true;
	}

	void AudioSystem::Tick()
	{
		s_pAudioDevice->Tick();
	}
}