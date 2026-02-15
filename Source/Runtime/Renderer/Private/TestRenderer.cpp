#pragma optimize("", off)
#include "TestRenderer.h"

#include "CoreModule.h"
#include "FrameGraph.h"
#include "MeshDrawCommand.h"
#include "MeshPass.h"
#include "MeshPassProcessor.h"
#include "PlatformProcess.h"
#include "RHICommand.h"
#include "Memory/TransientAllocator.h"
#include "PrimitiveSceneInfo.h"
#include "RenderModule.h"
#include "ShaderParameterMap.h"
#include "UniformBuffer.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "HAL/Event.h"

namespace Thunder
{
    TestRenderer::TestRenderer()
    {
    }

    void TestRenderer::Setup()
    {
        // GBuffer pass.
        static FGRenderTargetRef GBufferRT0 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
        static FGRenderTargetRef GBufferRT1 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };

        // Register render targets
        mFrameGraph->RegisterRenderTarget(GBufferRT0);
        mFrameGraph->RegisterRenderTarget(GBufferRT1);

        {
            PassOperations operations;
            operations.Write(GBufferRT0);
            operations.Write(GBufferRT1);
            mFrameGraph->AddPass(EVENT_NAME("GBufferPass"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDrawCommand* newCommand = new (context->Allocate<RHIDrawCommand>()) RHIDrawCommand;

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

                context->AddCommand(newCommand);
                Print("Execute gbuffer");
            });
        }

        // Lighting pass.
        static FGRenderTargetRef LightingRT = new FGRenderTarget{ 1920, 1080, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(LightingRT);

        {
            PassOperations operations;
            operations.Read(GBufferRT0);
            operations.Read(GBufferRT1);
            operations.Write(LightingRT);
            mFrameGraph->AddPass(EVENT_NAME("LightingPass"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute lighting");
            });
        }

        // PostProcess1 pass.
        static FGRenderTargetRef PostProcessRT1 = new FGRenderTarget{ 1280, 720, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(PostProcessRT1);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(PostProcessRT1);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess1"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess1");
            });
        }

        // PostProcess2 pass.
        static FGRenderTargetRef postProcessRT2 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
        mFrameGraph->RegisterRenderTarget(postProcessRT2);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(postProcessRT2);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess2"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess2");
            });
        }

        mFrameGraph->SetPresentTarget(postProcessRT2);
    }

    void TestRenderer::Tick_RenderThread()
    {
        mFrameGraph->UpdateSceneInfo_RenderThread();
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

    void DeferredShadingRenderer::Tick_RenderThread()
    {
        // Set global parameters and update uniform buffer.
        auto globalParameters = mFrameGraph->GetGlobalParameters();
        globalParameters->SetIntParameter("RenderQuality", 2);
        mFrameGraph->InitGlobalUniformBuffer();

        UpdateAllPrimitiveSceneInfos();
    }

    void DeferredShadingRenderer::Setup()
    {
        // GBuffer pass.
        static FGRenderTargetRef GBufferRT0 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
        static FGRenderTargetRef GBufferRT1 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
        static FGRenderTargetRef GBufferSceneDepth = new FGRenderTarget{ 1920, 1080, RHIFormat::D32_FLOAT_S8X24_UINT };

        // Register render targets
        mFrameGraph->RegisterRenderTarget(GBufferRT0);
        mFrameGraph->RegisterRenderTarget(GBufferRT1);
        mFrameGraph->RegisterRenderTarget(GBufferSceneDepth);

        {
            static FGRenderTargetRef DummyRT = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
            mFrameGraph->RegisterRenderTarget(DummyRT);

            PassOperations operations;
            operations.Write(DummyRT);
            operations.Write(GBufferSceneDepth);
            mFrameGraph->AddPass(EVENT_NAME("PrePass"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = mFrameGraph->GetPassParameters(EMeshPass::PrePass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                mFrameGraph->UpdatePassParameters(EMeshPass::PrePass, passParameters, "PrePass");

                MeshPassProcessor* processor = RenderModule::GetMeshPassProcessor(EMeshPass::PrePass);
                auto mainContext = mFrameGraph->GetMainContext();
                mFrameGraph->ResolveVisibility(EViewType::MainView, EMeshPass::PrePass);

                // Add cached mesh batches.
                TArray<RHICachedDrawCommand*> const& cachedDrawList = mFrameGraph->GetVisibleCachedDrawList(EMeshPass::PrePass);
                mainContext->AddCommandList(cachedDrawList);

                // Add dynamic mesh batches.
                auto mainView = mFrameGraph->GetSceneView(EViewType::MainView);
                for (auto& sceneInfo : mainView->GetVisibleDynamicSceneInfos())
                {
                    auto staticMeshes = sceneInfo->GetStaticMeshes();
                    for (const auto& batch : staticMeshes | std::views::values)
                    {
                        processor->AddMeshBatch(mainContext, batch, EMeshPass::PrePass);
                    }
                }

                Print("Execute PrePass");
            });
        }

        {
            static FGRenderTargetRef DummyRT = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
            mFrameGraph->RegisterRenderTarget(DummyRT);
            static FGRenderTargetRef ShadowDepth = new FGRenderTarget{ 1920, 1080, RHIFormat::D32_FLOAT_S8X24_UINT };
            mFrameGraph->RegisterRenderTarget(ShadowDepth);

            PassOperations operations;
            operations.Write(DummyRT);
            operations.Write(ShadowDepth);
            mFrameGraph->AddPass(EVENT_NAME("ShadowDepth"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = mFrameGraph->GetPassParameters(EMeshPass::ShadowPass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                mFrameGraph->UpdatePassParameters(EMeshPass::ShadowPass, passParameters, "ShadowDepth");

                auto context = mFrameGraph->GetMainContext();
                MeshPassProcessor* processor = RenderModule::GetMeshPassProcessor(EMeshPass::ShadowPass);
                auto shadowView = mFrameGraph->GetSceneView(EViewType::ShadowView);
                for (auto& sceneInfo : shadowView->GetVisibleDynamicSceneInfos())
                {
                    auto staticMeshes = sceneInfo->GetStaticMeshes();
                    for (const auto& batch : staticMeshes | std::views::values)
                    {
                        processor->AddMeshBatch(context, batch, EMeshPass::ShadowPass);
                    }
                }
                Print("Execute ShaderDepth");
            });
        }

        {
            PassOperations operations;
            operations.Write(GBufferRT0);
            operations.Write(GBufferRT1);
            operations.Write(GBufferSceneDepth);
            mFrameGraph->AddPass(EVENT_NAME("GBufferPass"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = mFrameGraph->GetPassParameters(EMeshPass::BasePass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                mFrameGraph->UpdatePassParameters(EMeshPass::BasePass, passParameters, "BasePass");

                auto mainContext = mFrameGraph->GetMainContext();
                mFrameGraph->ResolveVisibility(EViewType::MainView, EMeshPass::BasePass);

                // Add cached mesh batches.
                TArray<RHICachedDrawCommand*> const& cachedDrawList = mFrameGraph->GetVisibleCachedDrawList(EMeshPass::BasePass);
                mainContext->AddCommandList(cachedDrawList);

                // Add dynamic mesh batches.
                auto mainView = mFrameGraph->GetSceneView(EViewType::MainView);
                auto const& sceneInfos = mainView->GetVisibleDynamicSceneInfos();
                uint32 const sceneInfoCount = static_cast<uint32>(sceneInfos.size());
                if (sceneInfoCount > 0)
                {
                    const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
                    auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
                    dispatcher->Promise(static_cast<int>(sceneInfoCount));
                    uint32 numThread = static_cast<uint32>(mFrameGraph->GetRenderContexts().size());
                    GSyncWorkers->ParallelFor([this, &sceneInfos, dispatcher, sceneInfoCount](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
                    {
                        auto const& contexts = mFrameGraph->GetRenderContexts();
                        auto context = contexts[threadId];
                        MeshPassProcessor* processor = RenderModule::GetMeshPassProcessor(EMeshPass::BasePass);
                        for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                        {
                            if (index >= sceneInfoCount)
                            {
                                break;
                            }

                            auto sceneInfo = sceneInfos[index];
                            auto const& staticMeshes = sceneInfo->GetStaticMeshes();
                            for (const auto& batch : staticMeshes | std::views::values)
                            {
                                processor->AddMeshBatch(context, batch, EMeshPass::BasePass);
                            }

                            dispatcher->Notify();
                        }
                    }, sceneInfoCount, numThread);

                    doWorkEvent->Wait();
                    FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
                    TMemory::Destroy(dispatcher);
                }

                Print("Execute GBufferPass");
            });
        }

        // Lighting pass.
        static FGRenderTargetRef LightingRT = new FGRenderTarget{ 1920, 1080, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(LightingRT);

        {
            PassOperations operations;
            operations.Read(GBufferRT0);
            operations.Read(GBufferRT1);
            operations.Write(LightingRT);
            mFrameGraph->AddPass(EVENT_NAME("LightingPass"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute lighting");
            });
        }

        // PostProcess1 pass.
        static FGRenderTargetRef PostProcessRT1 = new FGRenderTarget{ 1280, 720, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(PostProcessRT1);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(PostProcessRT1);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess1"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess1");
            });
        }

        // PostProcess2 pass.
        static FGRenderTargetRef postProcessRT2 = new FGRenderTarget{ 1920, 1080, RHIFormat::R8G8B8A8_UNORM };
        mFrameGraph->RegisterRenderTarget(postProcessRT2);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(postProcessRT2);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess2"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
                Print("Execute postprocess2");
            });
        }

        mFrameGraph->SetPresentTarget(postProcessRT2);
    }

    void DeferredShadingRenderer::UpdateAllPrimitiveSceneInfos()
    {
        // Update scene info.
        mFrameGraph->UpdateSceneInfo_RenderThread();

        UpdatePrimitiveUniformBuffer_RenderThread();
    }
}
