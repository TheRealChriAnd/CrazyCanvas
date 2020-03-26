#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Console.h"

#include <stdarg.h>

#include "MacConsoleWindow.h"

#ifndef __OBJC__

class NSString;
class NSTextView;
class NSDictionary;
class NSScrollView;

#endif

namespace LambdaEngine
{
    class MacConsole : public Console
    {
    public:
        static void Show();
        static void Close();
        
        static void Print(const char* pFormat, ...);
        static void PrintLine(const char* pFormat, ...);
        
        static void Clear();

        static void SetColor(EConsoleColor color);
    
    private:
        MacConsole()    = default;
        ~MacConsole()   = default;
        
        void Init();
        void Release();
        
        void AppendTextAndScroll(NSString* pString);
        void NewLine();

        void PrintV(const char* pFormat, va_list args);
        
        MacConsoleWindow*   m_pWindow       = nullptr;
        NSTextView*         m_pTextView     = nullptr;
        NSScrollView*       m_pScrollView   = nullptr;
        NSDictionary*       m_pCurrentColor = nullptr;
        NSDictionary*       m_ppColors[4]   = { nullptr, nullptr, nullptr, nullptr };
        
        static MacConsole s_Console;
    };

    typedef MacConsole PlatformConsole;
}

#endif