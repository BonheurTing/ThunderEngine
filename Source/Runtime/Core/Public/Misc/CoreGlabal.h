#pragma once
#include <condition_variable>

namespace Thunder
{
    struct FrameState
    {
        std::atomic_uint32_t FrameNumberGameThread{0};
        std::atomic_uint32_t FrameNumberRenderThread{0};
        std::atomic_uint32_t FrameNumberRHIThread{0};

        std::mutex GameRenderMutex;
        std::condition_variable GameRenderCV;

        std::mutex RenderRHIMutex;
        std::condition_variable RenderRHICV;
    };
    
    extern CORE_API FrameState* GFrameState;
}

