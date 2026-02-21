#pragma once
#include "ApplicationWindow.export.h"
#include "Platform.h"

#if THUNDER_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace Thunder
{
    struct GameWindowDesc
    {
        uint32 Width  = 1280;
        uint32 Height = 720;
        const wchar_t* Title = L"ThunderGame";
    };

    class APPLICATIONWINDOW_API GameWindow
    {
    public:
        bool Create(const GameWindowDesc& desc);
        void Destroy();

        // Main thread message pump: processes all pending messages, returns false on WM_QUIT
        bool PumpMessages();

        HWND GetHWND() const { return Hwnd; }
        uint32 GetWidth()  const { return Width;  }
        uint32 GetHeight() const { return Height; }

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
        void OnMouseMove(int x, int y);
        void OnKeyDown(WPARAM vk);
        void OnKeyUp(WPARAM vk);

        HWND   Hwnd   = nullptr;
        uint32 Width  = 0;
        uint32 Height = 0;
        int    LastMouseX = 0;
        int    LastMouseY = 0;
        bool   FirstMouse = true;
    };
}
