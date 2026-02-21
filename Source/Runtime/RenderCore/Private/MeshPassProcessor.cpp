
#include "MeshPassProcessor.h"
#include "Memory/TransientAllocator.h"
#include "MeshPass.h"
#include "PrimitiveSceneInfo.h"
#include "RenderMaterial.h"
#include "RenderMesh.h"
//#include "UniformBuffer.h"
#include "FrameGraph.h"
#include "IDynamicRHI.h"
#include "RHICommand.h"
#include "RHI.h"
#include "ShaderModule.h"
#include "ShaderArchive.h"
#include "UniformBuffer.h"

namespace Thunder
{
    struct RHIDrawCommand;

    MeshPassProcessor::MeshPassProcessor()
    {
    }

    bool MeshPassProcessor::AddMeshBatch(RenderContext* context, const MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand)
    {
        return Process(context, batch, meshPassType, cacheMeshDrawCommand);
    }

    bool MeshPassProcessor::Process(RenderContext* context, const MeshBatch* batch, EMeshPass meshPassType, bool cacheMeshDrawCommand)
    {
        auto const& elements = batch->GetElements();
        for (auto const& meshBatchElement : elements)
        {
            RHIDrawCommand* newCommand = (cacheMeshDrawCommand) ?
                new (TMemory::Malloc<RHICachedDrawCommand>()) RHICachedDrawCommand :
                new (context->Allocate<RHIDrawCommand>()) RHIDrawCommand;
            RenderMaterial* material = meshBatchElement.Material;
            SubMesh* subMesh = meshBatchElement.SubMesh;

            // vb ib
            newCommand->VBToSet = subMesh->GetVerticesBuffer();
            newCommand->IBToSet = subMesh->GetIndicesBuffer();
            newCommand->VertexCount = static_cast<uint32>(subMesh->GetVertices()->GetDataNum());
            newCommand->IndexCount = static_cast<uint32>(subMesh->GetIndices()->GetDataNum());

            // Get shader variant first.
            ShaderCombination* shaderVariant = GetShaderCombination(meshPassType, material);
            if (!shaderVariant) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, Shader variant not found.");
                return false;
            }
            newCommand->Shader = shaderVariant;

            // Fetch PSO.
            TRHIGraphicsPipelineState* pso = GetPipelineState(context, shaderVariant, meshPassType, subMesh, material);
            if (!pso) [[unlikely]]
            {
                TAssertf(false, "Fail to prepare mesh draw command, pipeline state is invalid.");
                return false;
            }
            newCommand->GraphicsPSO = pso;

            // Apply shader bindings.
            ApplyShaderBindings(context, newCommand, shaderVariant, material, batch->GetSceneInfo(), meshPassType, cacheMeshDrawCommand);

            // Add mesh-draw command.
            FinalizeCommand(context, batch, newCommand, cacheMeshDrawCommand);
        }

        return true;
    }

    void MeshPassProcessor::FinalizeCommand(RenderContext* context, const MeshBatch* batch, RHIDrawCommand* command, bool cacheMeshDrawCommand)
    {
        if (cacheMeshDrawCommand)
        {
            context->AddCachedCommand(batch, static_cast<RHICachedDrawCommand*>(command));
        }
        else
        {
            context->AddCommand(command);
        }
    }

    ShaderCombination* MeshPassProcessor::GetShaderCombination(EMeshPass meshPassType, const RenderMaterial* material)
    {
        auto shaderAst = material->GetShaderArchive();
        uint64 shaderVariantMask = ShaderModule::GetVariantMask(shaderAst, material->GetStaticSwitchParameters());
        ShaderCombination* shaderVariant = ShaderModule::GetShaderCombination(shaderAst, meshPassType, shaderVariantMask);
        return shaderVariant;
    }

    TRHIGraphicsPipelineState* MeshPassProcessor::GetPipelineState(const RenderContext* context, ShaderCombination* shaderCombination, EMeshPass meshPassType, const SubMesh* subMesh, RenderMaterial* material)
    {
        // Set shader.
        TGraphicsPipelineStateDescriptor psoDesc;
        if (!shaderCombination) [[unlikely]]
        {
            // Shader is not ready yet.
            return nullptr;
        }
        psoDesc.shaderVariant = shaderCombination;

        // Get register counts.
        auto shaderAst = material->GetShaderArchive();
        bool succeeded = ShaderModule::GetPassRegisterCounts(shaderAst, meshPassType, psoDesc.RegisterCounts);
        if (!succeeded) [[unlikely]]
        {
            TAssertf(false, "Failed to get pipeline state, register count is invalid.");
            return nullptr;
        }

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

    void MeshPassProcessor::ApplyShaderBindings(RenderContext* context, RHIDrawCommand* command, ShaderCombination* shader, RenderMaterial* material, PrimitiveSceneInfo* sceneInfo, EMeshPass meshPassType, bool cacheMeshDrawCommand)
    {
        ShaderArchive* archive = shader->GetSubShader()->GetArchive();
        ShaderBindingsLayout* bindingsLayout = archive->GetBindingsLayout();
        command->Bindings.SetTransientAllocated(!cacheMeshDrawCommand);

        // Allocate.
        SingleShaderBindings* bindings = command->Bindings.GetSingleShaderBindings();
        size_t const bindingSize = bindingsLayout->GetTotalSize();
        byte* bindingData = (cacheMeshDrawCommand) ?
            static_cast<byte*>(TMemory::Malloc(bindingSize, 8)) :
            static_cast<byte*>(context->Allocate<byte>(bindingSize));
        memset(bindingData, 0, bindingSize);
        bindings->SetData(bindingData);

        // Update material uniform buffer.
        material->UpdateUniformBuffer(context, cacheMeshDrawCommand);

        // Bind Constant Buffer
        {
            // Global uniform buffer.
            auto frameGraph = context->GetFrameGraph();
            auto globalUB = frameGraph->GetGlobalUniformBuffer();
            if (globalUB == nullptr || globalUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get global CBV binding : buffer resource is invalid.");
            }

            static NameHandle globalUBName = "Global";
            bindings->SetUniformBuffer(bindingsLayout, globalUBName, { .Handle = reinterpret_cast<uint64>(globalUB) });

            // Pass uniform buffer.
            auto passUB = frameGraph->GetPassUniformBuffer(meshPassType);
            if (passUB == nullptr || passUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get pass \"%s\" CBV binding : buffer resource is invalid.", meshPassType);
            }

            static NameHandle passUBName = "Pass";
            bindings->SetUniformBuffer(bindingsLayout, passUBName, { .Handle = reinterpret_cast<uint64>(passUB) });
 
            // Material uniform buffer.
            const RHIUniformBuffer* materialUB = material->GetUniformBuffer();
            if (materialUB == nullptr || materialUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get CBV binding : buffer resource is invalid.");
            }

            static NameHandle materialUBName = "Material";
            bindings->SetUniformBuffer(bindingsLayout, materialUBName, { .Handle = reinterpret_cast<uint64>(materialUB) });

            // Primitive uniform buffer
            auto primitiveUB = sceneInfo->GetPrimitiveUniformBuffer();
            if (primitiveUB == nullptr || primitiveUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get global CBV binding : buffer resource is invalid.");
            }

            static NameHandle primitiveUBName = "Primitive";
            bindings->SetUniformBuffer(bindingsLayout, primitiveUBName, { .Handle = reinterpret_cast<uint64>(primitiveUB) });
        }

        // Bind textures.
        auto const& textureParameterMap = material->GetTextureParameters();
        for (const auto& textureParameterEntry : textureParameterMap)
        {
            NameHandle textureName = textureParameterEntry.first;
            TGuid textureGuid = textureParameterEntry.second;

            // Skip empty texture.
            if (!textureGuid.IsValid())
            {
                continue;
            }

            // Get resource.
            RHITexture* textureResource = material->GetTextureResource(textureName);
            if (!textureResource) [[unlikely]]
            {
                TAssertf(false, "Failed to get SRV binding : \"%s\", texture resource is invalid.", textureName.c_str());
                continue;
            }

            // Get SRV.
            RHIShaderResourceView* srv = textureResource->GetSRV();
            if (!srv) [[unlikely]]
            {
                TAssertf(false, "Failed to get SRV binding : \"%s\", SRV is invalid.", textureName.c_str());
                continue;
            }

            // Bind.
            bindings->SetSRV(bindingsLayout, textureName, { .Handle = srv->GetOfflineHandle() });
        }
    }
}
