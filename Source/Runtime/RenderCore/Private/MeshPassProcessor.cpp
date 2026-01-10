
#include "MeshPassProcessor.h"

#include "IDynamicRHI.h"
#include "MeshPass.h"
#include "PrimitiveSceneInfo.h"
#include "RenderMaterial.h"
#include "RHICommand.h"
#include "ShaderModule.h"
#include "Memory/TransientAllocator.h"

namespace Thunder
{
    struct RHIDrawCommand;

    MeshPassProcessor::MeshPassProcessor()
    {
    }

    bool MeshPassProcessor::AddMeshBatch(FRenderContext* context, const MeshBatch* batch, EMeshPass meshPassType)
    {
        return Process(context, batch, meshPassType);
    }

    bool MeshPassProcessor::Process(FRenderContext* context, const MeshBatch* batch, EMeshPass meshPassType)
    {
        auto const& elements = batch->GetElements();
        for (auto const& meshBatchElement : elements)
        {
            RenderMaterial* material = meshBatchElement.Material;
            SubMesh* subMesh = meshBatchElement.SubMesh;

            // Fetch shader.
            auto shaderAst = material->GetShaderArchive();
            uint64 shaderVariantMask = ShaderModule::GetVariantMask(shaderAst, material->GetStaticParameters());
            ShaderCombination* shaderVariant = ShaderModule::GetShaderCombination(shaderAst, meshPassType, shaderVariantMask);

            // Subshader->getRenderState(DepthStencilState, BlendState, RasterizationState)
            //[DepthStencilState, BlendState, RasterizationState] = subshader->getRenderState();
            // RenderTargets
            //RenderTargets = meshdrawpass->GetRenderTargetLayout();
            // inputLayout
            //InputLayout = FPrimitiveSceneInfo->GetInputLayout();

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

        return true;
    }
}
