#pragma once
#include "InputState.h"

#include "Application/API/Events/Event.h"

#include "Threading/API/SpinLock.h"

#include <stack>

namespace LambdaEngine
{
	#define STATE_READ_INDEX 0
	#define STATE_WRITE_INDEX 1

	enum class InputMode : uint8
	{
		GUI		= 0,
		GAME	= 1,
		NONE	= UINT8_MAX,
	};

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

		FORCEINLINE static void PushInputMode(InputMode inputMode)
		{
			s_InputModeStack.push(inputMode);
		}

		FORCEINLINE static void PopInputMode()
		{
			s_InputModeStack.pop();
		}

		FORCEINLINE static InputMode GetCurrentInputmode()
		{
			return s_InputModeStack.top();
		}

		static void Disable();

		FORCEINLINE static bool IsKeyDown(InputMode inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX].IsKeyDown(key);
		}

		FORCEINLINE static bool IsKeyUp(InputMode inputMode, EKey key)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX].IsKeyUp(key);
		}

		FORCEINLINE static bool IsInputEnabled()
		{
			return s_InputEnabled;
		}

		FORCEINLINE static const KeyboardState& GetKeyboardState(InputMode inputMode)
		{
			return s_KeyboardStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX];
		}

		FORCEINLINE static const MouseState& GetMouseState(InputMode inputMode)
		{
			return s_MouseStates[ConvertInputModeUINT8(inputMode)][STATE_READ_INDEX];
		}

	private:
		static bool HandleEvent(const Event& event);
		static uint8 ConvertInputModeUINT8(InputMode inputMode);

	private:
		// Input states are double buffered. The first one is read from, the second is written to.
		static KeyboardState s_KeyboardStates[2][2];
		static MouseState s_MouseStates[2][2];

		// Make sure nothing is being written to the write buffer when copying write buffer to read buffer in Input::Tick
		static SpinLock s_WriteBufferLockMouse;
		static SpinLock s_WriteBufferLockKeyboard;
		static std::atomic_bool s_InputEnabled;

		static std::stack<InputMode> s_InputModeStack;
	};
}
