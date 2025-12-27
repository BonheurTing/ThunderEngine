#include "RenderMaterial.h"
#include "ShaderArchive.h"
#include "ShaderDefinition.h"
#include "RHIResource.h"
#include "RHI.h"

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
        if (archive)
        {
            // 可以在这里进行一些初始化工作
        }
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

    RHITexture* RenderMaterial::GetTextureResource(const NameHandle& name) const
    {
        // TODO: 从TextureParameters的GUID查找已解析的纹理资源
        // 需要维护一个GUID到RHITexture的映射表
        return nullptr;
    }

    void RenderMaterial::ResolveTextureResources()
    {
        // TODO: 将TextureParameters中的GUID解析为实际的RHI纹理资源
        // 这需要通过资源管理器查找GUID对应的纹理资源

        // 示例代码框架：
        // for (const auto& pair : ParameterCache.TextureParameters)
        // {
        //     const TGuid& textureGuid = pair.second;
        //     // RHITexture* texture = ResourceManager->LoadTexture(textureGuid);
        //     // 将texture缓存到某个映射表中
        // }
    }

    void RenderMaterial::FillGraphicsPipelineStateDesc(
        TGraphicsPipelineStateDescriptor& outDesc,
        NameHandle passName,
        uint64 variantId) const
    {
        // 填充Shader相关信息
        outDesc.RegisterCounts = RegisterCounts;

        // 设置Shader标识符（用于查找编译好的Shader）
        outDesc.ShaderIdentifier = passName;

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

    const TMap<NameHandle, bool>& RenderMaterial::GetStaticParameters() const
    {
        return ParameterCache->StaticParameters;
    }
}
