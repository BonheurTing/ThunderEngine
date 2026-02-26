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
        RENDERCORE_API virtual void Reset() { FrameGraph->Reset(); }
        RENDERCORE_API virtual void Setup() = 0;

        RENDERCORE_API virtual void Compile()
        {
            if (FrameGraph)
            {
                FrameGraph->Compile();
            }
        }

        RENDERCORE_API virtual void Cull() {}

        RENDERCORE_API void Execute()
        {
            if (FrameGraph)
            {
                FrameGraph->Execute();
            }
        }

        RENDERCORE_API void EndRenderFrame()
        {
            if (FrameGraph)
            {
                FrameGraph->ClearRenderTargetPool();
            }
        }

        FORCEINLINE void RegisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { FrameGraph->RegisterSceneInfo_GameThread(sceneInfo); }
        FORCEINLINE void UnregisterSceneInfo(PrimitiveSceneInfo* sceneInfo) const { FrameGraph->UnregisterSceneInfo_GameThread(sceneInfo); }

        RENDERCORE_API void UpdatePrimitiveUniformBuffer_GameThread(PrimitiveSceneInfo* sceneInfo);
        RENDERCORE_API void UpdatePrimitiveUniformBuffer_RenderThread();

        FORCEINLINE FrameGraph* GetFrameGraph() const { return FrameGraph; }

    protected:
        TRefCountPtr<FrameGraph> FrameGraph;

        // Primitive uniform buffer update list
        TSet<PrimitiveSceneInfo*> PrimitiveUniformBufferUpdateSet[2]; // Game thread and render thread double buffer.
    };
}