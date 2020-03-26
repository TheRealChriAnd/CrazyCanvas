#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Application.h"

#include "MacWindow.h"
#include "MacAppController.h"

namespace LambdaEngine
{
    class MacApplication : public Application
    {
    public:
        MacApplication()    = default;
        ~MacApplication()   = default;

        virtual void AddMessageListner(IApplicationMessageHandler* pListener) override;
        
        virtual Window*         GetWindow()         override;
        virtual const Window*   GetWindow() const   override;
        
        bool Init();
        void Release();
        
        static bool PreInit();
        static bool PostRelease();
        
        static bool Tick();
        
        static void Terminate();
        
        FORCEINLINE static Application* Get()
        {
            return &s_Application;
        }
        
    private:
        MacWindow         m_Window;
        MacAppController* m_pAppDelegate    = nullptr;
        bool              m_IsTerminating   = false;
        
        static MacApplication s_Application;
    };

    typedef MacApplication PlatformApplication;
}

#endif
