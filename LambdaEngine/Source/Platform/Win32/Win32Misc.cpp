#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Win32/Win32Misc.h"
#include "Platform/Win32/Windows.h"

#ifdef MessageBox
	#undef MessageBox
#endif

namespace LambdaEngine
{
	void Win32Misc::MessageBox(const char* pCaption, const char* pText)
	{
		::MessageBoxA(0, pText, pCaption, MB_ICONERROR | MB_OK);
	}
}

#endif