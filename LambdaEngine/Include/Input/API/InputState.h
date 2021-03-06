#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	/*
	* KeyboardState
	*/

	struct KeyboardState
	{
		struct KeyState
		{
			uint8 IsDown		: 1;
			uint8 JustPressed	: 1;

			FORCEINLINE void Reset()
			{
				IsDown		= 0;
				JustPressed	= 0;
			}
		};

	public:
		FORCEINLINE bool IsKeyDown(EKey key) const
		{
			return KeyStates[key].IsDown;
		}

		FORCEINLINE bool IsKeyUp(EKey key) const
		{
			return !(KeyStates[key].IsDown);
		}

		FORCEINLINE bool IsKeyJustPressed(EKey key) const
		{
			return KeyStates[key].JustPressed;
		}

	public:
		KeyState KeyStates[EKey::KEY_COUNT];
	};

	/*
	* ModiferKeyState
	*/

	struct ModifierKeyState
	{
	public:
		FORCEINLINE ModifierKeyState()
			: ModiferMask(0)
		{
		}

		FORCEINLINE explicit ModifierKeyState(uint32 modiferMask)
			: ModiferMask(modiferMask)
		{
		}

		FORCEINLINE bool IsShiftDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_SHIFT);
		}

		FORCEINLINE bool IsAltDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_ALT);
		}

		FORCEINLINE bool IsCtrlDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_CTRL);
		}

		FORCEINLINE bool IsSuperDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_SUPER);
		}

		FORCEINLINE bool IsCapsLockDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_CAPS_LOCK);
		}

		FORCEINLINE bool IsNumLockDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_NUM_LOCK);
		}

	public:
		uint32 ModiferMask;
	};

	/*
	* MouseState
	*/

	struct MouseState
	{
	public:
		FORCEINLINE bool IsButtonPressed(EMouseButton button) const
		{
			return ButtonStates[button];
		}

		FORCEINLINE bool IsButtonReleased(EMouseButton button) const
		{
			return !ButtonStates[button];
		}

	public:
		struct
		{
			int32 x;
			int32 y;
		} Position;

		int32 ScrollX;
		int32 ScrollY;

		bool ButtonStates[EMouseButton::MOUSE_BUTTON_COUNT];
	};
}
