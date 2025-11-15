#pragma optimize("", off)
#include "TestRenderer.h"
#include "FrameGraph.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    DeferredShadingRenderer* GDeferredRenderer = nullptr;

    TestRenderer::TestRenderer()
    {
    }

    void TestRenderer::Setup()
    {
        if (!mFrameGraph)
        {
            return;
        }

        mFrameGraph->Reset();

        // GBuffer pass.
        FGRenderTarget GBufferRT0{ 1920, 1080, EPixelFormat::RGBA8888 };
        FGRenderTarget GBufferRT1{ 1920, 1080, EPixelFormat::RGBA8888 };

        // Register render targets
        mFrameGraph->RegisterRenderTarget(GBufferRT0);
        mFrameGraph->RegisterRenderTarget(GBufferRT1);

        {
            PassOperations operations;
            operations.write(GBufferRT0);
            operations.write(GBufferRT1);
            mFrameGraph->AddPass(EVENT_NAME("GBufferPass"), std::move(operations), [this](FRenderContext* Context)
            {
                RHIDrawCommand* newCommand = new (Context->GetTransientAllocator_RenderThread()->Allocate<RHIDrawCommand>()) RHIDrawCommand;

                // Set render resources
                //Command->VBToSet = VB;
                //Command->IBToSet = IB;
                //Command->GraphicsPSO = PSO;

                // Set draw parameters
                newCommand->IndexCount = 36;      // Example for a cube
                newCommand->InstanceCount = 1;
                newCommand->StartIndexLocation = 0;
                newCommand->BaseVertexLocation = 0;
                newCommand->StartInstanceLocation = 0;

                Context->AddCommand(newCommand);
                Print("Execute gbuffer");
            });
        }

        // Lighting pass.
        FGRenderTarget LightingRT{ 1920, 1080, EPixelFormat::RGBA16F };
        mFrameGraph->RegisterRenderTarget(LightingRT);

        {
            PassOperations operations;
            operations.read(GBufferRT0);
            operations.read(GBufferRT1);
            operations.write(LightingRT);
            mFrameGraph->AddPass(EVENT_NAME("LightingPass"), std::move(operations), [this](FRenderContext* context)
            {
                RHIDummyCommand* newCommand = new (context->GetTransientAllocator_RenderThread()->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute lighting");
            });
        }

        // PostProcess1 pass.
        FGRenderTarget PostProcessRT1{ 1280, 720, EPixelFormat::RGBA16F };
        mFrameGraph->RegisterRenderTarget(PostProcessRT1);

        {
            PassOperations operations;
            operations.read(LightingRT);
            operations.write(PostProcessRT1);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess1"), std::move(operations), [this](FRenderContext* context)
            {
                RHIDummyCommand* newCommand = new (context->GetTransientAllocator_RenderThread()->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess1");
            });
        }

        // PostProcess2 pass.
        FGRenderTarget postProcessRT2{ 1920, 1080, EPixelFormat::RGBA8888 };
        mFrameGraph->RegisterRenderTarget(postProcessRT2);

        {
            PassOperations operations;
            operations.read(LightingRT);
            operations.write(postProcessRT2);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess2"), std::move(operations), [this](FRenderContext* context)
            {
                RHIDummyCommand* newCommand = new (context->GetTransientAllocator_RenderThread()->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess2");
            });
        }

        mFrameGraph->SetPresentTarget(postProcessRT2);
    }

    void TestRenderer::Print(const std::string& message)
    {
        std::cout << message << std::endl;
    }

    DeferredShadingRenderer::DeferredShadingRenderer()
    {
    }

    void DeferredShadingRenderer::InitViews()
    {
        // scene 需要 build 一个 PrimitiveSceneInfo List，有primitiveid做索引，可以拿proxy

        
    }

    void DeferredShadingRenderer::Setup()
    {
        /*// comp 创建的时候需要创建对应的sceneproxy
        
        
        TArray<SceneView*> views = InitViews();
        Scene* currentScene;
        
        for (auto view : views)
        {
            TArray<MeshBatch*> batchCollector;
            for (each primitiveid)
            {
                PrimitiveSceneInfo* curPrimitiveSceneInfo = InScene->Primitives[primitiveid];
                MeshBatch meshBatch;
                curPrimitiveSceneInfo->Proxy->GetDynamicMeshElements(meshBatch); // 默认mesh都是每帧重画，即 Collect MeshBatch
                batchCollector.add(meshBatch);
            }
            
            for (auto MeshBatch : batchCollector)
            {
                ShaderArchive archive = MeshBatch->getrendermaterial->getshaderarchive // AST (包括很多subshader/pass)
                // get pso
            }

            // Lighting pass.
            // PostProcess pass.
        }*/
    }

    void DeferredShadingRenderer::Cull()
    {
        
    }
}
