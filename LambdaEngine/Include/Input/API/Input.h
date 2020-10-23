#pragma once
#include "InputState.h"

#include "Application/API/Events/Event.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	#define STATE_READ_INDEX 0
	#define STATE_WRITE_INDEX 1

	/*
	* Input
	*/
	class LAMBDA_API Input
	{
	public:
		DECL_STATIC_CLASS(Input);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static void Enable()
		{
			s_InputEnabled = true;
		}

		static void Disable();

		FORCEINLINE static bool IsKeyDown(EKey key)
		{
			return s_KeyboardStates[STATE_READ_INDEX].IsKeyDown(key);
		}

		FORCEINLINE static bool IsKeyUp(EKey key)
		{
			return s_KeyboardStates[STATE_READ_INDEX].IsKeyUp(key);
		}

		FORCEINLINE static bool IsInputEnabled()
		{
			return s_InputEnabled;
		}

		FORCEINLINE static const KeyboardState& GetKeyboardState()
		{
			return s_KeyboardStates[STATE_READ_INDEX];
		}

		FORCEINLINE static const MouseState& GetMouseState()
		{
			return s_MouseStates[STATE_READ_INDEX];
		}

	private:
		static bool HandleEvent(const Event& event);

	private:
		// Input states are double buffered. The first one is read from, the second is written to.
		static KeyboardState s_KeyboardStates[2];
		static MouseState s_MouseStates[2];
		// Make sure nothing is being written to the write buffer when copying write buffer to read buffer in Input::Tick
		static SpinLock s_WriteBufferLockMouse;
		static SpinLock s_WriteBufferLockKeyboard;
		static std::atomic_bool s_InputEnabled;
	};
}
