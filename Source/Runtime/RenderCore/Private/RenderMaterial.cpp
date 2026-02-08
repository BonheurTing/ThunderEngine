#include "RenderMaterial.h"

#include "IDynamicRHI.h"
#include "RenderTranslator.h"
#include "RenderTexture.h"
#include "RenderModule.h"
#include "ShaderArchive.h"
#include "ShaderDefinition.h"
#include "RHIResource.h"
#include "RHI.h"
#include "ShaderModule.h"
#include "ShaderBindingsLayout.h"
#include "RenderContext.h"

namespace Thunder
{
    RenderMaterial::RenderMaterial()
        : Archive(nullptr)
        , ParameterCache{}
        , UniformBuffer(nullptr)
    {
    }

    RenderMaterial::~RenderMaterial()
    {
        // RHI资源会通过RefCount自动释放
        
    }

    void RenderMaterial::SetShaderArchive(ShaderArchive* archive)
    {
        Archive = archive;
    }

    ShaderCombination* RenderMaterial::GetShaderCombination(NameHandle passName, uint64 variantId) const
    {
        if (!Archive)
        {
            return nullptr;
        }

        ShaderPass* pass = Archive->GetSubShader(passName);
        if (!pass)
        {
            return nullptr;
        }

        return pass->GetShaderCombination(variantId);
    }

    void RenderMaterial::CacheParameters(MaterialParameterCache* gameMaterialCache)
    {
        ParameterCache = gameMaterialCache; // todo : double buffer.
        bTexturesDirty = true; // Mark textures as dirty when parameters change
    }

    void RenderMaterial::UpdateUniformBuffer(const FRenderContext* context, bool cacheMeshDrawCommand)
    {
        if (!Archive || !ParameterCache) [[unlikely]]
        {
            TAssertf(false, "Cannot update uniform buffer: Archive or ParameterCache is null.");
            return;
        }

        // Get the uniform buffer layout for "material" CB.
        UniformBufferLayout* ubLayout = Archive->GetUniformBufferLayout("Material");
        if (!ubLayout) [[unlikely]]
        {
            TAssertf(false, "Cannot update uniform buffer: UniformBufferLayout \"material\" not found in archive \"%s\".", Archive->GetName().c_str());
            return;
        }

        uint32 const bufferSize = ubLayout->GetTotalSize();
        if (bufferSize == 0) [[unlikely]]
        {
            // No uniform buffer needed.
            return;
        }

        // Allocate temporary buffer for packing.
        byte* packedData = (cacheMeshDrawCommand) ?
            static_cast<byte*>(TMemory::Malloc(bufferSize, 16)) :
            static_cast<byte*>(context->Allocate<byte>(bufferSize));
        memset(packedData, 0, bufferSize);

        // Pack parameters into the buffer according to layout.
        auto const& memberMap = ubLayout->GetMemberMap();
        for (auto const& [paramName, memberEntry] : memberMap)
        {
            void const* valuePtr = nullptr;
            uint32 valueSize = 0;

            // Find parameter value based on type.
            switch (memberEntry.Type)
            {
            case EUniformBufferMemberType::Int:
            {
                auto it = ParameterCache->IntParameters.find(paramName);
                if (it != ParameterCache->IntParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(int32);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in IntParameters for material \"%s\".", paramName.c_str(), Archive->GetName().c_str());
                }
                break;
            }
            case EUniformBufferMemberType::Float:
            {
                auto it = ParameterCache->FloatParameters.find(paramName);
                if (it != ParameterCache->FloatParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(float);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in FloatParameters for material \"%s\".", paramName.c_str(), Archive->GetName().c_str());
                }
                break;
            }
            case EUniformBufferMemberType::Float4:
            {
                auto it = ParameterCache->VectorParameters.find(paramName);
                if (it != ParameterCache->VectorParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(TVector4f);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in VectorParameters for material \"%s\".", paramName.c_str(), Archive->GetName().c_str());
                }
                break;
            }
            default:
                TAssertf(false, "Unsupported uniform buffer member type for parameter \"%s\".", paramName.c_str());
                break;
            }

            // Copy value to buffer.
            if (valuePtr && valueSize > 0) [[likely]]
            {
                TAssertf(valueSize == memberEntry.Size, "Parameter \"%s\" size mismatch: expected %u, got %u.",
                    paramName.c_str(), memberEntry.Size, valueSize);
                memcpy(packedData + memberEntry.Offset, valuePtr, valueSize);
            }
        }

        // Create or update RHI constant buffer.
        // Todo : dynamic draw commands should use a transient allocator.
        if (!UniformBuffer)
        {
            // Create new constant buffer.
            UniformBuffer = RHICreateConstantBuffer(bufferSize, EBufferCreateFlags::Dynamic); // Maybe on default heap?
            if (!UniformBuffer) [[unlikely]]
            {
                TAssertf(false, "Failed to create constant buffer for material \"%s\".", Archive->GetName().c_str());
                if (cacheMeshDrawCommand)
                {
                    TMemory::Free(packedData);
                }
                return;
            }

            // Create constant buffer view.
            RHICreateConstantBufferView(*UniformBuffer.Get(), bufferSize);
        }

        // Update buffer data.
        /*
        bool succeeded = RHIUpdateSharedMemoryResource(UniformBuffer.Get(), packedData, bufferSize, 0);
        if (!succeeded) [[unlikely]]
        {
            TAssertf(false, "Failed to update constant buffer for material \"%s\".", Archive->GetName().c_str());
        }

        // Free temporary buffer if needed.
        if (cacheMeshDrawCommand)
        {
            TMemory::Free(packedData);
        }
        */
    }

    RHITexture* RenderMaterial::GetTextureResource(const NameHandle& name)
    {
        if (bTexturesDirty) [[unlikely]]
        {
            CacheTextureResources();
        }

        auto it = TextureCaches.find(name);
        if (it != TextureCaches.end()) [[likely]]
        {
            return it->second;
        }
        return nullptr;
    }

    void RenderMaterial::CacheTextureResources()
    {
        if (!bTexturesDirty || !ParameterCache)
        {
            return;
        }

        TextureCaches.clear();

        for (auto const& [name, guid] : ParameterCache->TextureParameters)
        {
            if (!guid.IsValid())
            {
                // Empty GUIDs are allowed, just skip them.
                continue;
            }
            RenderTexture* renderTexture = RenderModule::GetTextureResource_RenderThread(guid);
            if (renderTexture) [[likely]]
            {
                RHITexture* rhiTexture = renderTexture->GetTextureRHI().Get();
                if (rhiTexture) [[likely]]
                {
                    TextureCaches[name] = rhiTexture;
                }
                else
                {
                    TAssertf(false, "Texture \"%s\" (GUID: %s) has no RHI resource.", name.c_str(), guid.ToString().c_str());
                }
            }
            else
            {
                TAssertf(false, "Texture \"%s\" (GUID: %s) not found in render texture registry.", name.c_str(), guid.ToString().c_str());
            }
        }

        bTexturesDirty = false;
    }

    void RenderMaterial::FillGraphicsPipelineStateDesc(
        TGraphicsPipelineStateDescriptor& outDesc,
        NameHandle passName,
        uint64 variantId) const
    {
    }

    void RenderMaterial::BindParametersToRHI(class RHICommandList* cmdList) const
    {
        // TODO: 绑定Constant Buffer和Texture到渲染管线

        // 示例代码框架：
        // if (ConstantBuffer)
        // {
        //     cmdList->SetConstantBuffer(0, ConstantBuffer.Get());
        // }
        //
        // 遍历TextureParameters，绑定纹理资源
        // for (const auto& pair : ParameterCache.TextureParameters)
        // {
        //     // 根据GUID查找对应的RHITexture
        //     // RHITexture* texture = GetTextureResource(pair.first);
        //     // if (texture)
        //     // {
        //     //     cmdList->SetShaderResourceView(slot, texture->GetSRV());
        //     // }
        // }
    }

    bool RenderMaterial::GetRenderState(EMeshPass meshPassType, RHIBlendState& outBlendState, RHIRasterizerState& outRasterizerState, RHIDepthStencilState& outDepthStencilState)
    {
        // Get sub-shader.
        ShaderPass* subShader = Archive->GetSubShader(meshPassType);
        if (!subShader) [[unlikely]]
        {
            TAssertf(false, "Sub-shader \"%s\" not found in archive \"%s\".", ShaderModule::GetMeshPassName(meshPassType).c_str(), Archive->GetName().c_str());
            return false;
        }

        // Blend state : currently we only have opaque and transparent modes.
        outBlendState = RHIBlendState{}; // Reset.
        switch (subShader->GetBlendMode())
        {
        case EShaderBlendMode::Translucent:
            {
                for (uint32 targetIndex = 0u; targetIndex < MAX_RENDER_TARGETS; ++targetIndex)
                {
                    RHIBlendDescriptor& targetBlend = outBlendState.RenderTarget[targetIndex];
                    targetBlend.BlendEnable = 1;
                }
            }
            break;
        case EShaderBlendMode::Opaque:
        case EShaderBlendMode::Unknown:
        default:
            break; // Use default behavior.
        }

        // Rasterization state.
        outRasterizerState = RHIRasterizerState{};

        // Depth stencil state.
        outDepthStencilState = RHIDepthStencilState{};
        switch (subShader->GetDepthState())
        {
        case EShaderDepthState::Never:
            outDepthStencilState.DepthFunc = ERHIComparisonFunc::Never;
            break;
        case EShaderDepthState::Less:
            outDepthStencilState.DepthFunc = ERHIComparisonFunc::Less;
            break;
        case EShaderDepthState::Equal:
            outDepthStencilState.DepthFunc = ERHIComparisonFunc::Equal;
            break;
        case EShaderDepthState::Greater:
            outDepthStencilState.DepthFunc = ERHIComparisonFunc::Greater;
            break;
        case EShaderDepthState::Always:
            outDepthStencilState.DepthFunc = ERHIComparisonFunc::Always;
            break;
        case EShaderDepthState::Unknown:
        default:
            break; // Use default behavior.
        }
        switch (subShader->GetStencilState())
        {
        case EShaderStencilState::Default:
        case EShaderStencilState::Unknown:
            break; // Use default behavior.
        }

        return true;
    }
}
