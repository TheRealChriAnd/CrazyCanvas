#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"
#include "Audio/FMOD/AudioGeometryFMOD.h"
#include "Audio/FMOD/ReverbSphereFMOD.h"

#include "Log/Log.h"

#include "Threading/API/Thread.h"

namespace LambdaEngine
{
	FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS severity, const char* pFile, int line, const char* pFunc, const char* pMessage)
	{
		if (severity & FMOD_DEBUG_LEVEL_WARNING)
		{
			LOG_WARNING("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (severity & FMOD_DEBUG_LEVEL_ERROR)
		{
			LOG_ERROR("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (!(severity & FMOD_DEBUG_LEVEL_LOG))
		{
			D_LOG_INFO("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}

		return FMOD_OK;
	}

	AudioDeviceFMOD::AudioDeviceFMOD() :
		pSystem(nullptr)
	{
		
	}

	AudioDeviceFMOD::~AudioDeviceFMOD()
	{
		if (m_pMusicHandle != nullptr)
		{
			FMOD_Channel_Stop(m_pMusicChannel);
			FMOD_Sound_Release(m_pMusicHandle);
			m_pMusicHandle = nullptr;
		}
		
		if (pSystem != nullptr)
		{
			if (FMOD_System_Release(pSystem) != FMOD_OK)
			{
				LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be released for %s", m_pName);
			}
			else
			{
				D_LOG_MESSAGE("[AudioDeviceFMOD]: FMOD System released successfully for %s", m_pName);
			}

			pSystem = nullptr;
		}
	}

	bool AudioDeviceFMOD::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;
		m_MaxNumAudioListeners = pDesc->MaxNumAudioListeners;

		if (pDesc->Debug)
		{
			FMOD_DEBUG_FLAGS debugLevel		= FMOD_DEBUG_LEVEL_LOG;
			FMOD_DEBUG_MODE debugMode		= FMOD_DEBUG_MODE_CALLBACK;

			if (FMOD_Debug_Initialize(debugLevel, debugMode, DebugCallback, nullptr) != FMOD_OK)
			{
				LOG_WARNING("[AudioDeviceFMOD]: FMOD Debug Mode could not be initialized");
				return false;
			}
		}

		if (FMOD_System_Create(&pSystem) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be created for %s", m_pName);
			return false;
		}

		int numChannels				= 512;
		FMOD_INITFLAGS initFlags	= FMOD_INIT_NORMAL;

		if (FMOD_System_Init(pSystem, numChannels, initFlags, nullptr) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be initialized for %s", m_pName);
			return false;
		}

		if (FMOD_System_SetGeometrySettings(pSystem, pDesc->MaxWorldSize) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD Geometry Settings could not be set for %s", m_pName);
			return false;
		}

		D_LOG_MESSAGE("[AudioDeviceFMOD]: Successfully initialized %s!", m_pName);

		return true;
	}

	void AudioDeviceFMOD::Tick()
	{
		FMOD_System_Update(pSystem);
	}

	bool AudioDeviceFMOD::LoadMusic(const char* pFilepath)
	{
		if (m_pMusicHandle != nullptr)
		{
			FMOD_Sound_Release(m_pMusicHandle);
			m_pMusicHandle = nullptr;
		}
		
		FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		
		if (FMOD_System_CreateSound(pSystem, pFilepath, mode, &soundCreateInfo, &m_pMusicHandle) != FMOD_OK)
		{
			LOG_WARNING("[AudioDeviceFMOD]: Music \"%s\" could not be initialized in %s", pFilepath, m_pName);
			return false;
		}

		if (FMOD_System_PlaySound(pSystem, m_pMusicHandle, nullptr, true, &m_pMusicChannel) != FMOD_OK)
		{
			LOG_WARNING("[AudioDeviceFMOD]: Music \"%s\" could not be played in %s", pFilepath, m_pName);
			return false;
		}

		D_LOG_MESSAGE("[AudioDeviceFMOD]: Loaded Music \"%s\" in %s", pFilepath, m_pName);
		
		return true;
	}

	void AudioDeviceFMOD::PlayMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 0);
		}
	}

	void AudioDeviceFMOD::PauseMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 1);
		}
	}

	void AudioDeviceFMOD::ToggleMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_BOOL isPaused = 0;
			FMOD_Channel_GetPaused(m_pMusicChannel, &isPaused);
			FMOD_Channel_SetPaused(m_pMusicChannel, (isPaused ^ 0x1));
		}
	}

	void AudioDeviceFMOD::UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc)
	{
		FMOD_VECTOR fmodPosition	= { pDesc->Position.x,		pDesc->Position.y,		pDesc->Position.z };
		FMOD_VECTOR fmodVelocity	= { 0.0f,					0.0f,					0.0f };
		FMOD_VECTOR fmodForward		= { pDesc->Forward.x,		pDesc->Forward.y,		pDesc->Forward.z };
		FMOD_VECTOR fmodUp			= { pDesc->Up.x,			pDesc->Up.y,			pDesc->Up.z };

		FMOD_System_Set3DListenerAttributes(pSystem, index, &fmodPosition, &fmodVelocity, &fmodForward, &fmodUp);
	}

	uint32 AudioDeviceFMOD::CreateAudioListener()
	{
		if (m_NumAudioListeners >= m_MaxNumAudioListeners)
		{
			LOG_WARNING("[AudioDeviceFMOD]: Audio Listener could not be created, max amount reached for %s!", m_pName);
			return UINT32_MAX;
		}

		return m_NumAudioListeners++;
	}

	ISoundEffect3D* AudioDeviceFMOD::CreateSoundEffect(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect3DFMOD* pSoundEffect = DBG_NEW SoundEffect3DFMOD(this);

		if (!pSoundEffect->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundEffect;
		}
	}

	ISoundInstance3D* AudioDeviceFMOD::CreateSoundInstance(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DFMOD* pSoundInstance = DBG_NEW SoundInstance3DFMOD(this);

		if (!pSoundInstance->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundInstance;
		}
	}

	IAudioGeometry* AudioDeviceFMOD::CreateAudioGeometry(const AudioGeometryDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		AudioGeometryFMOD* pAudioGeometry = DBG_NEW AudioGeometryFMOD(this);

		if (!pAudioGeometry->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pAudioGeometry;
		}
	}

	IReverbSphere* AudioDeviceFMOD::CreateReverbSphere(const ReverbSphereDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		ReverbSphereFMOD* pReverbSphere = DBG_NEW ReverbSphereFMOD(this);

		if (!pReverbSphere->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pReverbSphere;
		}
	}

	void AudioDeviceFMOD::SetMasterVolume(float volume)
	{
		LOG_WARNING("[AudioDeviceFMOD]: SetMasterVolume called but not implemented!");
	}

	float AudioDeviceFMOD::GetMasterVolume() const
	{
		LOG_WARNING("[AudioDeviceFMOD]: GetMasterVolume called but not implemented!");
		return 0;
	}
};
