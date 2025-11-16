#pragma optimize("", off)
#include "TestRenderer.h"
#include "FrameGraph.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"
#include "PrimitiveSceneProxy.h"

namespace Thunder
{
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

    void DeferredShadingRenderer::Print(const std::string& message)
    {
        std::cout << message << std::endl;
    }

    void DeferredShadingRenderer::Setup()
    {
        if (!mFrameGraph)
        {
            return;
        }

        mFrameGraph->Reset();

        // GBuffer pass.
        FGRenderTarget GBufferRT0{ 1920, 1080, EPixelFormat::RGBA8888 };
        FGRenderTarget GBufferRT1{ 1920, 1080, EPixelFormat::RGBA8888 };
        FGRenderTarget GBufferSceneDepth{ 1920, 1080, EPixelFormat::D32S8X24 };

        // Register render targets
        mFrameGraph->RegisterRenderTarget(GBufferRT0);
        mFrameGraph->RegisterRenderTarget(GBufferRT1);
        mFrameGraph->RegisterRenderTarget(GBufferSceneDepth);

        {
            FGRenderTarget DummyRT{ 1920, 1080, EPixelFormat::RGBA8888 };
            mFrameGraph->RegisterRenderTarget(DummyRT);

            PassOperations operations;
            operations.write(DummyRT);
            operations.write(GBufferSceneDepth);
            mFrameGraph->AddPass(EVENT_NAME("PrePass"), std::move(operations), [this](FRenderContext* Context)
            {
                auto mainView = mFrameGraph->GetSceneView(EViewType::MainView);
                for (auto& sceneProxy : mainView->GetVisibleSceneProxies())
                {
                    sceneProxy->AddDrawCall();
                }
                Print("Execute PrePass");
            });
        }

        {
            FGRenderTarget DummyRT{ 1920, 1080, EPixelFormat::RGBA8888 };
            mFrameGraph->RegisterRenderTarget(DummyRT);
            FGRenderTarget ShadowDepth{ 1920, 1080, EPixelFormat::D32S8X24 };
            mFrameGraph->RegisterRenderTarget(ShadowDepth);
            
            PassOperations operations;
            operations.write(DummyRT);
            operations.write(ShadowDepth);
            mFrameGraph->AddPass(EVENT_NAME("ShaderDepth"), std::move(operations), [this](FRenderContext* Context)
            {
                auto mainView = mFrameGraph->GetSceneView(EViewType::ShadowView);
                for (auto& sceneProxy : mainView->GetVisibleSceneProxies())
                {
                    sceneProxy->AddDrawCall();
                }
                Print("Execute ShaderDepth");
            });
        }

        {
            PassOperations operations;
            operations.write(GBufferRT0);
            operations.write(GBufferRT1);
            operations.write(GBufferSceneDepth);
            mFrameGraph->AddPass(EVENT_NAME("GBufferPass"), std::move(operations), [this](FRenderContext* Context)
            {
                auto mainView = mFrameGraph->GetSceneView(EViewType::MainView);
                for (auto& sceneProxy : mainView->GetVisibleSceneProxies())
                {
                    sceneProxy->AddDrawCall();
                }
                Print("Execute GBufferPass");
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
}
