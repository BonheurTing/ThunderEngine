#pragma optimize("", off)
#include "TestRenderer.h"

#include "CoreModule.h"
#include "FrameGraph.h"
#include "FrameGraphUtils.h"
#include "MeshPass.h"
#include "MeshPassProcessor.h"
#include "PlatformProcess.h"
#include "PostProcessRenderer.h"
#include "RHICommand.h"
#include "PrimitiveSceneInfo.h"
#include "RenderMesh.h"
#include "RenderModule.h"
#include "ShaderParameterMap.h"
#include "Concurrent/ConcurrentBase.h"
#include "Concurrent/TaskScheduler.h"
#include "HAL/Event.h"

namespace Thunder
{
    namespace
    {
        void Print(const std::string& message)
        {
            std::cout << message << '\n';
        }
    }

    void TestRenderer::Setup()
    {
        uint32 const resolutionWidth = GDynamicRHI->GetViewportWidth();
        uint32 const resolutionHeight = GDynamicRHI->GetViewportHeight();
        
        // GBuffer pass.
        static FGRenderTargetRef GBufferRT0 = new FGRenderTarget{"GBufferA", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };
        static FGRenderTargetRef GBufferRT1 = new FGRenderTarget{"GBufferB", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };

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
            });
        }

        // Lighting pass.
        static FGRenderTargetRef LightingRT = new FGRenderTarget{"SceneTexture", resolutionWidth, resolutionHeight, RHIFormat::R16G16B16A16_FLOAT };
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
            });
        }

        // PostProcess1 pass.
        static FGRenderTargetRef PostProcessRT1 = new FGRenderTarget{"SceneTexture1", 1280, 720, RHIFormat::R16G16B16A16_FLOAT };
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
            });
        }

        {
            // PostProcess2 (present pass): reads LightingRT, renders directly to the swapchain backbuffer.
            PassOperations operations;
            operations.Read(LightingRT);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess2"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
            });
            mFrameGraph->SetPresentPass("PostProcess2");
        }
    }

    void TestRenderer::Tick_RenderThread()
    {
        mFrameGraph->UpdateSceneInfo_RenderThread();
    }



    DeferredShadingRenderer::DeferredShadingRenderer()
    {
    }

    void DeferredShadingRenderer::InitViews()
    {
        // todo The scene needs to build a PrimitiveSceneInfo list.
        // Each element is indexed by PrimitiveId and can be used to access its proxy.
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
        uint32 const resolutionWidth = GDynamicRHI->GetViewportWidth();
        uint32 const resolutionHeight = GDynamicRHI->GetViewportHeight();

        // GBuffer pass.
        static FGRenderTargetRef GBufferRT0 = new FGRenderTarget{"GBufferA", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM, TVector4f(1, 1, 0, 1) };
        static FGRenderTargetRef GBufferRT1 = new FGRenderTarget{ "GBufferB", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };
        static FGRenderTargetRef GBufferSceneDepth = new FGRenderTarget{"SceneDepth",  resolutionWidth, resolutionHeight, RHIFormat::D32_FLOAT_S8X24_UINT };

        // Register render targets
        mFrameGraph->RegisterRenderTarget(GBufferRT0);
        mFrameGraph->RegisterRenderTarget(GBufferRT1);
        mFrameGraph->RegisterRenderTarget(GBufferSceneDepth);

        {
            static FGRenderTargetRef DummyRT = new FGRenderTarget{"SceneDepth1", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };
            mFrameGraph->RegisterRenderTarget(DummyRT);

            PassOperations operations;
            operations.Write(DummyRT);
            operations.Write(GBufferSceneDepth, 1.f, 0);
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
            });
        }

        {
            static FGRenderTargetRef DummyRT = new FGRenderTarget{"Dummy", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };
            mFrameGraph->RegisterRenderTarget(DummyRT);
            static FGRenderTargetRef ShadowDepth = new FGRenderTarget{"ShadowDepth", resolutionWidth, resolutionHeight, RHIFormat::D32_FLOAT_S8X24_UINT };
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
            });
        }

        {
            PassOperations operations;
            operations.Write(GBufferRT0, TVector4f(0, 0, 0, 1));
            operations.Write(GBufferRT1, TVector4f(0, 0, 0, 1));
            operations.Write(GBufferSceneDepth, 0, 0);
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
            });
        }

        // Lighting pass.
        static FGRenderTargetRef LightingRT = new FGRenderTarget{"SceneTexture", resolutionWidth, resolutionHeight, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(LightingRT);

        {
            PassOperations operations;
            operations.Read(GBufferRT0);
            operations.Read(GBufferRT1);
            operations.Write(LightingRT);
            mFrameGraph->AddPass(EVENT_NAME("Lighting"), std::move(operations), [this]()
            {
                auto pass = mFrameGraph->GetMainContext()->GetCurrentPass();
                bool success = pass->SetShader("Lighting");
                if (!success) [[unlikely]]
                {
                    TAssertf(false, "Failed to set shader.");
                    return;
                }

                pass->PassParameters->SetFloatParameter("PassParameters0", 0);
                pass->PassParameters->SetVectorParameter("PassParameters1", TVector4f(0.f, 0.f, 0.f, 1.f));
                pass->PassParameters->SetVectorParameter("PassParameters2", TVector4f(0.f, 0.f, 0.f, 1.f));
                pass->PassParameters->SetIntParameter("PassParameters3", 1);

                auto subMesh = GProceduralGeometryManager->GetGeometry(EProceduralGeometry::Triangle);
                RenderModule::BuildDrawCommand(mFrameGraph, pass, subMesh);
            });
        }

        // PostProcess1 pass.
        static FGRenderTargetRef PostProcessRT1 = new FGRenderTarget{"PostProcessRT1", 1280, 720, RHIFormat::R16G16B16A16_FLOAT };
        mFrameGraph->RegisterRenderTarget(PostProcessRT1);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(PostProcessRT1);
            mFrameGraph->AddPass(EVENT_NAME("PostProcess1"), std::move(operations), [this]()
            {
                auto context = mFrameGraph->GetMainContext();
                FrameGraphPass* currentPass = context->GetCurrentPass();
                
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
            });
        }

        {
            // Blur (present pass): reads LightingRT and renders directly to the swapchain backbuffer.
            // No write target is declared; the framegraph binds the backbuffer automatically.
            PassOperations operations;
            operations.Read(LightingRT, "SceneTexture"); //todo bind

            static PostProcessManager* ppRenderer = nullptr;
            if (ppRenderer == nullptr)
            {
                ppRenderer = new PostProcessManager(mFrameGraph);
            }

            ppRenderer->Setup(operations);
            mFrameGraph->SetPresentPass("Blur");
        }
    }

    void DeferredShadingRenderer::UpdateAllPrimitiveSceneInfos()
    {
        // Update scene info.
        mFrameGraph->UpdateSceneInfo_RenderThread();

        UpdatePrimitiveUniformBuffer_RenderThread();
    }
}
