#include "RenderMaterial.h"

#include "RenderTranslator.h"
#include "RenderTexture.h"
#include "RenderModule.h"
#include "ShaderArchive.h"
#include "ShaderDefinition.h"
#include "RHIResource.h"
#include "RHI.h"
#include "ShaderModule.h"

namespace Thunder
{
    RenderMaterial::RenderMaterial()
        : Archive(nullptr)
        , RegisterCounts{}
        , ParameterCache{}
        , ConstantBuffer(nullptr)
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
        ParameterCache = gameMaterialCache;
        bTexturesDirty = true; // Mark textures as dirty when parameters change
    }

    void RenderMaterial::UpdateConstantBuffer()
    {
        // TODO: 根据ShaderArchive的参数元数据，将参数打包到ConstantBufferData
        // 这需要从ShaderArchive获取参数布局信息，然后按照布局打包数据

        // 示例代码框架：
        // if (CompiledShaderMap)
        // {
        //     // 1. 获取参数元数据
        //     // 2. 计算CB大小
        //     // 3. 按照offset打包数据到ConstantBufferData
        //     // 4. 创建或更新RHI ConstantBuffer
        // }
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
        // 填充Shader相关信息
        outDesc.RegisterCounts = RegisterCounts;

        // TODO: 从ShaderArchive获取RenderState并填充到PSO
        // if (CompiledShaderMap)
        // {
        //     const RenderStateMeta& renderState = CompiledShaderMap->GetRenderState();
        //     // 根据renderState填充BlendState, RasterizerState, DepthStencilState等
        // }
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
