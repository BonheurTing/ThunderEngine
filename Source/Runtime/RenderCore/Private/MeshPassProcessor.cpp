
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
            RHIDrawCommand* newCommand = new (context->Allocate<RHIDrawCommand>()) RHIDrawCommand;
            RenderMaterial* material = meshBatchElement.Material;
            SubMesh* subMesh = meshBatchElement.SubMesh;

            // Fetch PSO.
            TRHIGraphicsPipelineState* pso = GetPipelineState(context, meshPassType, subMesh, material);
            if (!pso) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, pipeline state is invalid.");
                return false;
            }
            newCommand->GraphicsPSO = pso;

            context->AddCommand(newCommand);
        }

        return true;
    }

    TRHIGraphicsPipelineState* MeshPassProcessor::GetPipelineState(const FRenderContext* context, EMeshPass meshPassType, const SubMesh* subMesh, RenderMaterial* material)
    {
        // Fetch shader.
        auto shaderAst = material->GetShaderArchive();
        uint64 shaderVariantMask = ShaderModule::GetVariantMask(shaderAst, material->GetStaticParameters());
        ShaderCombination* shaderVariant = ShaderModule::GetShaderCombination(shaderAst, meshPassType, shaderVariantMask);
        if (!shaderVariant) [[unlikely]]
        {
            // Shader is not ready yet.
            return nullptr;
        }

        // Get register counts.
        TGraphicsPipelineStateDescriptor psoDesc;
        bool succeeded = ShaderModule::GetPassRegisterCounts(shaderAst, meshPassType, psoDesc.RegisterCounts);
        if (!succeeded) [[unlikely]]
        {
            TAssertf(false, "Failed to get pipeline state, register count is invalid.");
            return nullptr;
        }
        psoDesc.shaderVariant = shaderVariant;

        // Get vertex declaration.
        succeeded = subMesh->GetVertexDeclaration(psoDesc.VertexDeclaration);
        if (!succeeded) [[unlikely]]
        {
            TAssertf(false, "Failed to get pipeline state, vertex declaration is invalid.");
            return nullptr;
        }

        // Get pass.
        psoDesc.Pass = context->GetRenderPass();
        if (!psoDesc.Pass) [[unlikely]]
        {
            TAssertf(false, "Failed to get pipeline state, render pass is invalid.");
            return nullptr;
        }

        // Get render state.
        succeeded = material->GetRenderState(meshPassType, psoDesc.BlendState, psoDesc.RasterizerState, psoDesc.DepthStencilState);
        if (!succeeded) [[unlikely]]
        {
            TAssertf(false, "Failed to get pipeline state, render state is invalid.");
            return nullptr;
        }

        auto pipelineStateObject = RHICreateGraphicsPipelineState(psoDesc);
        return pipelineStateObject;
    }
}
