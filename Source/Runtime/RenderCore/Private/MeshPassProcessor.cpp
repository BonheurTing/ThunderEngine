
#include "MeshPassProcessor.h"

#include "IDynamicRHI.h"
#include "MeshPass.h"
#include "PrimitiveSceneInfo.h"
#include "RenderMaterial.h"
#include "RHICommand.h"
#include "ShaderModule.h"
#include "Memory/TransientAllocator.h"
#include "Container/ReflectiveContainer.h"
#include "BasicDefinition.h"
#include "RenderMesh.h"
#include "RenderTranslator.h"
#include "RHI.h"

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
            if (!shaderVariant) [[unlikely]]
            {
                // Shader is not ready yet.
                return false;
            }

            // Get register counts.
            TGraphicsPipelineStateDescriptor psoDesc;
            bool succeeded = ShaderModule::GetPassRegisterCounts(shaderAst, meshPassType, psoDesc.RegisterCounts);
            if (!succeeded) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, register count is invalid.");
                return false;
            }
            psoDesc.shaderVariant = shaderVariant;

            // Get vertex declaration.
            succeeded = subMesh->GetVertexDeclaration(psoDesc.VertexDeclaration);
            if (!succeeded) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, vertex declaration is invalid.");
                return false;
            }

            // Get pass.
            psoDesc.Pass = context->GetRenderPass();
            if (!psoDesc.Pass) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, render pass is invalid.");
                return false;
            }

            // psoDesc.RenderTargetFormats[0] = RHIFormat::R8G8B8A8_UNORM; // context->GetMeshDrawPass(meshDrawType)->GetOutput()[0].Format;
            // psoDesc.DepthStencilFormat = RHIFormat::UNKNOWN; // context->GetMeshDrawPass(meshDrawType)->GetDepthOutput().Format;
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
