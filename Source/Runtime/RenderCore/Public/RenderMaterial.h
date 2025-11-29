#pragma once
#include "CoreMinimal.h"
#include "NameHandle.h"
#include "Container.h"
#include "Vector.h"
#include "Guid.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    class ShaderArchive;
    class ShaderCombination;
    class RHITexture;
    class RHIConstantBuffer;
    struct TShaderRegisterCounts;
    struct TGraphicsPipelineStateDescriptor;
    class GameMaterial;

    struct MaterialParameterCache
    {
        TMap<NameHandle, int32> IntParameters;
        TMap<NameHandle, float> FloatParameters;
        TMap<NameHandle, TVector4f> VectorParameters;
        TMap<NameHandle, TGuid> TextureParameters;
        TMap<NameHandle, bool> StaticParameters;
    };

    // FMaterialResource + FMaterialRenderProxy
    class RenderMaterial
    {
    public:
        RenderMaterial();
        ~RenderMaterial();

        // ========== Shader ==========
        ShaderArchive* GetShaderArchive() const { return CompiledShaderMap; }
        void SetShaderArchive(ShaderArchive* archive);
        ShaderCombination* GetShaderCombination(NameHandle passName, uint64 variantId = 0) const;

        const TShaderRegisterCounts* GetShaderRegisterCounts() const { return &RegisterCounts; }
        void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { RegisterCounts = counts; }

        // ========== GameThread -> RenderThread ==========
        void CacheParameters(MaterialParameterCache* gameMaterialCache);

        // ========== GPU Resource (RenderThread) ==========
        void UpdateConstantBuffer();
        RHIConstantBuffer* GetConstantBuffer() const { return ConstantBuffer.Get(); }
        RHITexture* GetTextureResource(const NameHandle& name) const;
        void ResolveTextureResources();

        // ========== PSO (RenderThread) ==========
        void FillGraphicsPipelineStateDesc(
            TGraphicsPipelineStateDescriptor& outDesc,
            NameHandle passName,
            uint64 variantId = 0) const;

        void BindParametersToRHI(class RHICommandList* cmdList) const;
        const MaterialParameterCache& GetParameterCache() const { return ParameterCache; }
        const TMap<NameHandle, bool>& GetStaticParameters() { return ParameterCache.StaticParameters; }

    private:
        ShaderArchive* CompiledShaderMap { nullptr }; // shader module manages lifetime
        TShaderRegisterCounts RegisterCounts {};
        MaterialParameterCache ParameterCache;

        TRefCountPtr<RHIConstantBuffer> ConstantBuffer;
    };

}