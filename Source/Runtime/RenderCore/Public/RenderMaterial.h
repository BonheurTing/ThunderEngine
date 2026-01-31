#pragma once
#include "CoreMinimal.h"
#include "NameHandle.h"
#include "Container.h"
#include "Vector.h"
#include "Guid.h"
#include "MeshPass.h"
#include "RHI.h"
#include "ShaderDefinition.h"
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
    class MaterialParameterCache;

    // FMaterialResource + FMaterialRenderProxy
    class RenderMaterial
    {
    public:
        RenderMaterial();
        ~RenderMaterial();

        // ========== Shader ==========
        ShaderArchive* GetShaderArchive() const { return Archive; }
        void SetShaderArchive(ShaderArchive* archive);
        ShaderCombination* GetShaderCombination(NameHandle passName, uint64 variantId = 0) const;

        const TShaderRegisterCounts* GetShaderRegisterCounts() const { return &QuantizedRegisterCounts; }
        void SetShaderRegisterCounts(const TShaderRegisterCounts& counts) { QuantizedRegisterCounts = counts; }

        // ========== GameThread -> RenderThread ==========
        void CacheParameters(MaterialParameterCache* gameMaterialCache);

        // ========== GPU Resource (RenderThread) ==========
        void UpdateConstantBuffer();
        RHIConstantBuffer* GetConstantBuffer() const { return ConstantBuffer.Get(); }
        RHITexture* GetTextureResource(const NameHandle& name);
        void CacheTextureResources();

        // ========== PSO (RenderThread) ==========
        void FillGraphicsPipelineStateDesc(
            TGraphicsPipelineStateDescriptor& outDesc,
            NameHandle passName,
            uint64 variantId = 0) const;

        void BindParametersToRHI(class RHICommandList* cmdList) const;
        const MaterialParameterCache* GetParameterCache() const { return ParameterCache; }
        const TMap<NameHandle, bool>& GetStaticSwitchParameters() const { return ParameterCache->StaticParameters; }
        const TMap<NameHandle, TVector4f>& GetVectorParameters() const { return ParameterCache->VectorParameters; }
        const TMap<NameHandle, TGuid>& GetTextureParameters() const { return ParameterCache->TextureParameters; }
        const TMap<NameHandle, float>& GetFloatParameters() const { return ParameterCache->FloatParameters; }
        const TMap<NameHandle, int>& GetIntParameters() const { return ParameterCache->IntParameters; }

        bool GetRenderState(EMeshPass meshPassType, RHIBlendState& outBlendState, RHIRasterizerState& outRasterizerState, RHIDepthStencilState& outDepthStencilState);

    private:
        ShaderArchive* Archive { nullptr }; // shader module manages lifetime
        TShaderRegisterCounts QuantizedRegisterCounts {};
        MaterialParameterCache* ParameterCache; //

        TRefCountPtr<RHIConstantBuffer> ConstantBuffer;

        // Cached resolved textures (render thread only)
        TMap<NameHandle, RHITexture*> TextureCaches;
        bool bTexturesDirty = true;
    };

}