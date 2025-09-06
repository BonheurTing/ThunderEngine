#pragma once
#include <condition_variable>

namespace Thunder
{
    struct FrameState {
        std::atomic<int> FrameNumberGameThread{0};      // GameThread 的当前帧
        std::atomic<int> FrameNumberRenderThread{0};    // RenderThread 的当前帧
        std::atomic<int> FrameNumberRHIThread{0};       // RHIThread 的当前帧

        std::mutex GameRenderMutex;         // GameThread 和 RenderThread 的同步锁
        std::condition_variable GameRenderCV; // GameThread 和 RenderThread 的条件变量

        std::mutex RenderRHIMutex;          // RenderThread 和 RHIThread 的同步锁
        std::condition_variable RenderRHICV; // RenderThread 和 RHIThread 的条件变量
    };
    
    extern CORE_API FrameState* GFrameState;
}

