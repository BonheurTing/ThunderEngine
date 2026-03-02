#pragma optimize("", off)
#include "DeferredRenderer.h"

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
#include "Concurrent/TheadPool.h"
#include "HAL/Event.h"

namespace Thunder
{
    DeferredRenderer::DeferredRenderer()
    {
    }

    DeferredRenderer::~DeferredRenderer()
    {
    }

    void DeferredRenderer::InitViews()
    {
        // todo The scene needs to build a PrimitiveSceneInfo list.
        // Each element is indexed by PrimitiveId and can be used to access its proxy.
    }

    void DeferredRenderer::Tick_RenderThread()
    {
        // Set global parameters and update uniform buffer.
        auto globalParameters = FrameGraph->GetGlobalParameters();
        globalParameters->SetIntParameter("RenderQuality", 2);

        FrameGraph->InitGlobalUniformBuffer();
        UpdateAllPrimitiveSceneInfos();
    }

    void DeferredRenderer::Setup()
    {
        TVector2u viewportResolution = GDynamicRHI->GetMainViewportResolution_RenderThread();
        uint32 const resolutionWidth = viewportResolution.X;
        uint32 const resolutionHeight = viewportResolution.Y;
        FrameGraph->SetViewportResolution(viewportResolution);

        // GBuffer pass.
        static FGRenderTargetRef GBufferRT0 = new FGRenderTarget{"GBufferA", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM, TVector4f(0, 0, 0, 1) };
        static FGRenderTargetRef GBufferRT1 = new FGRenderTarget{ "GBufferB", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM, TVector4f(0, 0, 0, 1) };
        static FGRenderTargetRef GBufferRT2 = new FGRenderTarget{ "GBufferC", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM, TVector4f(0, 0, 0, 1) };
        static FGRenderTargetRef GBufferRT3 = new FGRenderTarget{ "GBufferD", resolutionWidth, resolutionHeight, RHIFormat::R32_FLOAT, TVector4f(0, 0, 0, 1) };
        static FGRenderTargetRef GBufferSceneDepth = new FGRenderTarget{"SceneDepth",  resolutionWidth, resolutionHeight, RHIFormat::D32_FLOAT_S8X24_UINT, 0.f, 0 };

        // Register render targets
        FrameGraph->RegisterRenderTarget(GBufferRT0, viewportResolution);
        FrameGraph->RegisterRenderTarget(GBufferRT1, viewportResolution);
        FrameGraph->RegisterRenderTarget(GBufferRT2, viewportResolution);
        FrameGraph->RegisterRenderTarget(GBufferRT3, viewportResolution);
        FrameGraph->RegisterRenderTarget(GBufferSceneDepth, viewportResolution);

        {
            static FGRenderTargetRef DummyRT = new FGRenderTarget{"SceneDepth1", resolutionWidth, resolutionHeight, RHIFormat::R8G8B8A8_UNORM };
            FrameGraph->RegisterRenderTarget(DummyRT, viewportResolution);

            PassOperations operations;
            operations.Write(DummyRT);
            operations.Write(GBufferSceneDepth, 0.f, 0);
            FrameGraph->AddPass(EVENT_NAME("PrePass"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = FrameGraph->GetPassParameters(EMeshPass::PrePass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                FrameGraph->UpdatePassParameters(EMeshPass::PrePass, passParameters, "PrePass");

                MeshPassProcessor* processor = RenderModule::GetMeshPassProcessor(EMeshPass::PrePass);
                auto mainContext = FrameGraph->GetMainContext();
                FrameGraph->ResolveVisibility(EViewType::MainView, EMeshPass::PrePass);

                // Add cached mesh batches.
                TArray<RHICachedDrawCommand*> const& cachedDrawList = FrameGraph->GetVisibleCachedDrawList(EMeshPass::PrePass);
                mainContext->AddCommandList(cachedDrawList);

                // Add dynamic mesh batches.
                auto mainView = FrameGraph->GetSceneView(EViewType::MainView);
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
            FrameGraph->RegisterRenderTarget(DummyRT, viewportResolution);
            static FGRenderTargetRef ShadowDepth = new FGRenderTarget{"ShadowDepth", resolutionWidth, resolutionHeight, RHIFormat::D32_FLOAT_S8X24_UINT };
            FrameGraph->RegisterRenderTarget(ShadowDepth, viewportResolution);

            PassOperations operations;
            operations.Write(DummyRT);
            operations.Write(ShadowDepth);
            FrameGraph->AddPass(EVENT_NAME("ShadowDepth"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = FrameGraph->GetPassParameters(EMeshPass::ShadowPass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                FrameGraph->UpdatePassParameters(EMeshPass::ShadowPass, passParameters, "ShadowDepth");

                auto context = FrameGraph->GetMainContext();
                MeshPassProcessor* processor = RenderModule::GetMeshPassProcessor(EMeshPass::ShadowPass);
                auto shadowView = FrameGraph->GetSceneView(EViewType::ShadowView);
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
            operations.Write(GBufferRT2, TVector4f(0, 0, 0, 1));
            operations.Write(GBufferRT3, TVector4f(0, 0, 0, 1));
            operations.Write(GBufferSceneDepth, 0, 0);
            FrameGraph->AddPass(EVENT_NAME("GBufferPass"), std::move(operations), [this]()
            {
                // Set pass parameters and update uniform buffer.
                auto passParameters = FrameGraph->GetPassParameters(EMeshPass::BasePass);
                passParameters->SetVectorParameter("PassParameters0", TVector4f(1,2,3,1));
                FrameGraph->UpdatePassParameters(EMeshPass::BasePass, passParameters, "BasePass");

                auto mainContext = FrameGraph->GetMainContext();
                FrameGraph->ResolveVisibility(EViewType::MainView, EMeshPass::BasePass);

                // Add cached mesh batches.
                TArray<RHICachedDrawCommand*> const& cachedDrawList = FrameGraph->GetVisibleCachedDrawList(EMeshPass::BasePass);
                mainContext->AddCommandList(cachedDrawList);

                // Add dynamic mesh batches.
                auto mainView = FrameGraph->GetSceneView(EViewType::MainView);
                auto const& sceneInfos = mainView->GetVisibleDynamicSceneInfos();
                uint32 const sceneInfoCount = static_cast<uint32>(sceneInfos.size());
                if (sceneInfoCount > 0)
                {
                    const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
                    auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
                    dispatcher->Promise(static_cast<int>(sceneInfoCount));
                    GSyncWorkers->ParallelFor([this, &sceneInfos, dispatcher, sceneInfoCount](uint32 bundleBegin, uint32 bundleSize)
                    {
                        auto const& contexts = FrameGraph->GetRenderContexts();
                        auto context = contexts[GetContextId()];
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
                    }, sceneInfoCount);

                    doWorkEvent->Wait();
                    FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
                    TMemory::Destroy(dispatcher);
                }
            });
        }

        // Lighting pass.
        static FGRenderTargetRef LightingRT = new FGRenderTarget{"SceneTexture", resolutionWidth, resolutionHeight, RHIFormat::R16G16B16A16_FLOAT, TVector4f(0, 0, 0, 1) };
        FrameGraph->RegisterRenderTarget(LightingRT, viewportResolution);

        {
            PassOperations operations;
            operations.Read(GBufferRT0);
            operations.Read(GBufferRT1);
            operations.Read(GBufferRT2);
            operations.Read(GBufferRT3);
            operations.Write(LightingRT);
            FrameGraph->AddPass(EVENT_NAME("Lighting"), std::move(operations), [this]()
            {
                auto pass = FrameGraph->GetMainContext()->GetCurrentPass();
                pass->SetShader("Lighting");

                pass->PassParameters->SetFloatParameter("PassParameters0", 0);
                pass->PassParameters->SetVectorParameter("PassParameters1", TVector4f(0.f, 0.f, 0.f, 1.f));
                pass->PassParameters->SetVectorParameter("PassParameters2", TVector4f(0.f, 0.f, 0.f, 1.f));
                pass->PassParameters->SetIntParameter("PassParameters3", 1);

                auto subMesh = GProceduralGeometryManager->GetGeometry(EProceduralGeometry::Triangle);
                RenderModule::BuildDrawCommand(FrameGraph, pass, subMesh);
            });
        }

        // PostProcess1 pass.
        static FGRenderTargetRef PostProcessRT1 = new FGRenderTarget{"PostProcessRT1", resolutionWidth, resolutionHeight, RHIFormat::R16G16B16A16_FLOAT };
        FrameGraph->RegisterRenderTarget(PostProcessRT1, viewportResolution);

        {
            PassOperations operations;
            operations.Read(LightingRT);
            operations.Write(PostProcessRT1);
            FrameGraph->AddPass(EVENT_NAME("PostProcess1"), std::move(operations), [this]()
            {
                auto context = FrameGraph->GetMainContext();
                FrameGraphPass* currentPass = context->GetCurrentPass();
                
                RHIDummyCommand* newCommand = new (context->Allocate<RHIDummyCommand>()) RHIDummyCommand;
                context->AddCommand(newCommand);
            });
        }

        {
            if (PostProcessManager == nullptr)
            {
                PostProcessManager = new class PostProcessManager(FrameGraph);
            }
            PostProcessManager->Render(LightingRT.Get());
        }

        FrameGraph->SetPresentPass("ToneMapping");
    }

    void DeferredRenderer::UpdateAllPrimitiveSceneInfos()
    {
        // Update scene info.
        FrameGraph->UpdateSceneInfo_RenderThread();

        UpdatePrimitiveUniformBuffer_RenderThread();
    }
}
