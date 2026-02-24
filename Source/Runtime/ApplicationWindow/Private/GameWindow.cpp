#include "GameWindow.h"
#include "InputState.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
    InputState GInputState;

    bool GameWindow::Create(const GameWindowDesc& desc)
    {
        Width  = desc.Width;
        Height = desc.Height;

        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(WNDCLASSEXW);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"ThunderGameWindow";

        if (!RegisterClassExW(&wc))
        {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }
        }

        RECT rect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        Hwnd = CreateWindowExW(
            0,
            L"ThunderGameWindow",
            desc.Title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr,
            GetModuleHandleW(nullptr),
            this);

        if (!Hwnd)
        {
            return false;
        }

        SetWindowLongPtrW(Hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        ShowWindow(Hwnd, SW_SHOW);
        UpdateWindow(Hwnd);
        return true;
    }

    void GameWindow::Destroy()
    {
        if (Hwnd)
        {
            DestroyWindow(Hwnd);
            Hwnd = nullptr;
        }
        UnregisterClassW(L"ThunderGameWindow", GetModuleHandleW(nullptr));
    }

    bool GameWindow::PumpMessages()
    {
        MSG msg = {};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return true;
    }

    void GameWindow::CallResizeCallback(uint32 width, uint32 height) const
    {
        if (OnResizeCallback && width > 0 && height > 0)
        {
            GGameScheduler->PushTask([this, width, height]()
            {
                OnResizeCallback(width, height);
            });
        }
    }

    LRESULT CALLBACK GameWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        GameWindow* self = reinterpret_cast<GameWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (msg)
        {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (self)
                self->OnKeyDown(wp);
            return 0;

        case WM_KEYUP:
            if (self)
                self->OnKeyUp(wp);
            return 0;

        case WM_MOUSEMOVE:
            if (self)
            {
                int x = static_cast<int>(LOWORD(lp));
                int y = static_cast<int>(HIWORD(lp));
                self->OnMouseMove(x, y);
            }
            return 0;

        case WM_RBUTTONDOWN:
            GInputState.RightButtonDown = true;
            return 0;

        case WM_RBUTTONUP:
            GInputState.RightButtonDown = false;
            return 0;

        case WM_LBUTTONDOWN:
            GInputState.LeftButtonDown = true;
            return 0;

        case WM_LBUTTONUP:
            GInputState.LeftButtonDown = false;
            return 0;

        case WM_SIZE:
            if (self && wp != SIZE_MINIMIZED)
            {
                uint32 newWidth = LOWORD(lp);
                uint32 newHeight = HIWORD(lp);
                if (newWidth > 0 && newHeight > 0 && (newWidth != self->Width || newHeight != self->Height))
                {
                    self->Width = newWidth;
                    self->Height = newHeight;
                    self->CallResizeCallback(newWidth, newHeight);
                }
            }
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
        }
    }

    void GameWindow::OnMouseMove(int x, int y)
    {
        if (FirstMouse)
        {
            LastMouseX = x;
            LastMouseY = y;
            FirstMouse = false;
        }

        GInputState.MouseDelta.X += static_cast<float>(x - LastMouseX);
        GInputState.MouseDelta.Y += static_cast<float>(y - LastMouseY);
        GInputState.MousePos.X = static_cast<float>(x);
        GInputState.MousePos.Y = static_cast<float>(y);

        LastMouseX = x;
        LastMouseY = y;
    }

    void GameWindow::OnKeyDown(WPARAM vk)
    {
        if (vk < 256)
        {
            GInputState.Keys[static_cast<uint8>(vk)] = true;
        }
    }

    void GameWindow::OnKeyUp(WPARAM vk)
    {
        if (vk < 256)
        {
            GInputState.Keys[static_cast<uint8>(vk)] = false;
        }
    }
}
