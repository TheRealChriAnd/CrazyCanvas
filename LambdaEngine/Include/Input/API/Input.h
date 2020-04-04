#pragma once
#include "InputDevice.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
    class IMouseHandler;
    class IKeyboardHandler;

	class LAMBDA_API Input
	{
	public:
		DECL_STATIC_CLASS(Input);

		static bool Init();
		static void Release();

		static void Update();

		static void AddKeyboardHandler(IKeyboardHandler* pHandler);
		static void AddMouseHandler(IMouseHandler* pHandler);

		static void SetInputMode(EInputMode inputMode);

	private:
		static IInputDevice* s_pInputDevice;

		static KeyboardState	s_KeyboardState;
		static MouseState		s_MouseState;
	};
}
