#pragma once

#include "RenderCore.export.h"
#include "FrameGraph.h"

namespace Thunder
{
    class IRenderer
    {
    public:
        RENDERCORE_API IRenderer();
        RENDERCORE_API virtual ~IRenderer();

        RENDERCORE_API virtual void Tick_RenderThread() = 0;
        RENDERCORE_API virtual void Reset() { mFrameGraph->Reset(); }
        RENDERCORE_API virtual void Setup() = 0;

        RENDERCORE_API virtual void Compile()
        {
            if (mFrameGraph)
            {
                mFrameGraph->Compile();
            }
        }

        RENDERCORE_API virtual void Cull() {}

        RENDERCORE_API void Execute()
        {
            if (mFrameGraph)
            {
                mFrameGraph->Execute();
            }
        }

        RENDERCORE_API void EndRenderFrame()
        {
            if (mFrameGraph)
            {
                mFrameGraph->ClearRenderTargetPool();
            }
        }

        FORCEINLINE void RegisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { mFrameGraph->RegisterSceneInfo_GameThread(sceneInfo); }
        FORCEINLINE void UnregisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { mFrameGraph->UnregisterSceneInfo_GameThread(sceneInfo); }

        RENDERCORE_API void UpdatePrimitiveUniformBuffer_GameThread(PrimitiveSceneInfo* sceneInfo);
        RENDERCORE_API void UpdatePrimitiveUniformBuffer_RenderThread();

        FORCEINLINE FrameGraph* GetFrameGraph() const { return mFrameGraph; }

    protected:
        TRefCountPtr<FrameGraph> mFrameGraph;

        // Primitive uniform buffer update list
        TSet<PrimitiveSceneInfo*> PrimitiveUniformBufferUpdateSet[2]; // Game thread and render thread double buffer.
    };
}