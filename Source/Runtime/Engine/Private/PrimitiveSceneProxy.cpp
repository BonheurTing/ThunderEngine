#include "PrimitiveSceneProxy.h"
#include "HAL/Event.h"
#include "Compomemt.h"
#include "FrameGraph.h"
#include "IDynamicRHI.h"
#include "PipelineStateCache.h"
#include "RenderMaterial.h"
#include "RHICommand.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "Misc/CoreGlabal.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    PrimitiveSceneProxy::PrimitiveSceneProxy(const PrimitiveComponent* inComponent)
    {
        const TMap<NameHandle, IMaterial*> gameMaterials = inComponent->GetMaterials();
        RenderMaterials.reserve(gameMaterials.size());
        for (const auto& val : gameMaterials | std::views::values)
        {
            RenderMaterials.push_back(val->GetMaterialResource());
        }
    }

    void PrimitiveSceneProxy::AddDrawCall(FRenderContext* context, EMeshPass meshPassType)
    {
        for (auto material : RenderMaterials)
        {
            // Fetch shader.
            auto shaderAst = material->GetShaderArchive();
            uint64 shaderVariantMask = ShaderModule::GetVariantMask(shaderAst, material->GetStaticParameters());
            ShaderCombination* shaderVariant = ShaderModule::GetShaderCombination(shaderAst, meshPassType, shaderVariantMask);

            // Subshader->getRenderState(DepthStencilState, BlendState, RasterizationState)
            //[DepthStencilState, BlendState, RasterizationState] = subshader->getRenderState();
            // RenderTargets
            //RenderTargets = meshdrawpass->GetRenderTargetLayout();
            // inputLayout
            //InputLayout = FPrimitiveSceneProxy->GetInputLayout();

            TGraphicsPipelineStateDescriptor psoDesc;
            bool ret = ShaderModule::GetPassRegisterCounts(shaderAst, meshPassType, psoDesc.RegisterCounts);
            TAssertf(ret, "Fail to get register count.");
            TArray<RHIVertexElement> inputElements =
            {
                { ERHIVertexInputSemantic::Position, 0, RHIFormat::R32G32B32_FLOAT, 0, 0, false },
                { ERHIVertexInputSemantic::TexCoord, 0, RHIFormat::R32G32_FLOAT, 0, 12, false }
            };
            psoDesc.VertexDeclaration = RHIVertexDeclarationDescriptor{ inputElements }; // GetMesh()->GetVertexDeclaration();
            psoDesc.shaderVariant = shaderVariant;
            psoDesc.RenderTargetFormats[0] = RHIFormat::R8G8B8A8_UNORM; // context->GetMeshDrawPass(meshDrawType)->GetOutput()[0].Format;
            psoDesc.DepthStencilFormat = RHIFormat::UNKNOWN; // context->GetMeshDrawPass(meshDrawType)->GetDepthOutput().Format;
            // psoDesc.Pass = context->GetMeshDrawPass(meshDrawType)->GetPass();
            psoDesc.NumSamples = 1;
            psoDesc.PrimitiveType = ERHIPrimitiveType::Triangle; // Check(GetMesh()->GetPrimitiveType() == E_Triangle);
            psoDesc.RenderTargetsEnabled = 1;

            //SetGraphicsPipelineState(context, psoDesc); //sync
            
            //RHIBlendState					BlendState; // subShader->GetBlendState();
            //RHIRasterizerState			    RasterizerState; // subShader->GetRasterizerState();
            //RHIDepthStencilState            DepthStencilState; // subShader->GetRasterizerState();

            auto pipelineStateObject = RHICreateGraphicsPipelineState(psoDesc); //sync
            if (pipelineStateObject)
            {
                RHIDrawCommand* newCommand = new (context->GetTransientAllocator_RenderThread()->Allocate<RHIDrawCommand>()) RHIDrawCommand;
                newCommand->GraphicsPSO = pipelineStateObject;
                context->AddCommand(newCommand);
            }
        }
    }

    StaticMeshSceneProxy::StaticMeshSceneProxy(StaticMeshComponent* inComponent)
        : PrimitiveSceneProxy(inComponent)
    {
        
    }

    void SceneView::CullSceneProxies()
    {
        VisibleSceneProxies.clear();

        auto& proxySet = OwnerFrameGraph->GetSceneProxies();
        TArray<PrimitiveSceneProxy*> allProxies( proxySet.begin(), proxySet.end() );
        uint32 proxyNum = static_cast<uint32>(allProxies.size());
        if (proxyNum > 0)
        {
            TArray<TArray<PrimitiveSceneProxy*>> LocalVisibleProxies(GSyncWorkers->GetNumThreads());

            const auto doWorkEvent = FPlatformProcess::GetSyncEventFromPool();
            auto* dispatcher = new (TMemory::Malloc<TaskDispatcher>()) TaskDispatcher(doWorkEvent);
            dispatcher->Promise(static_cast<int>(proxyNum));
            GSyncWorkers->ParallelFor([&allProxies, &LocalVisibleProxies, dispatcher, proxyNum, this](uint32 bundleBegin, uint32 bundleSize, uint32 threadId)
            {
                for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
                {
                    if (index >= proxyNum) break;

                    if (allProxies[index]->NeedRenderView(ViewType))
                    {
                        if (FrustumCull(allProxies[index]))
                        {
                            LocalVisibleProxies[threadId].push_back(allProxies[index]);
                        }
                    }

                    dispatcher->Notify();
                }
            }, proxyNum);

            doWorkEvent->Wait();
            FPlatformProcess::ReturnSyncEventToPool(doWorkEvent);
            TMemory::Destroy(dispatcher);

            // integrate
            size_t totalSize = 0;
            for (const auto& proxies : LocalVisibleProxies) {
                totalSize += proxies.size();
            }
            VisibleSceneProxies.reserve(totalSize);
            for (const auto& proxies : LocalVisibleProxies) {
                VisibleSceneProxies.insert(VisibleSceneProxies.end(), proxies.begin(), proxies.end());
            }
        }
    }


    TArray<PrimitiveSceneProxy*>& SceneView::GetVisibleSceneProxies()
    {
        //TODO main loop
        if (!IsCulled())
        {
            CullSceneProxies();

            uint32 frameNum = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire);
            CurrentFrameCulled.store(frameNum, std::memory_order_release);
        }
        return VisibleSceneProxies;
    }
}
