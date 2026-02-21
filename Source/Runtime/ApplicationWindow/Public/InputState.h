#pragma once
#include "ApplicationWindow.export.h"
#include "BasicDefinition.h"
#include "Vector.h"

namespace Thunder
{
    struct InputState
    {
        // Keyboard: W A S D Q E + Shift/Escape
        bool Keys[256] = {};

        // Mouse
        TVector2f MouseDelta { 0.f, 0.f };   // Per-frame delta (pixels)
        TVector2f MousePos   { 0.f, 0.f };   // Absolute position
        bool      RightButtonDown = false;
        bool      LeftButtonDown  = false;

        FORCEINLINE bool IsKeyDown(uint8 vk) const { return Keys[vk]; }
    };

    extern APPLICATIONWINDOW_API InputState GInputState;  // Written by main thread, read by game thread
}
