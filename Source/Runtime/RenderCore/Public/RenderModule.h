#pragma once
#include "Container.h"
#include "MeshPass.h"
#include "Module/ModuleManager.h"
#include <array>
#include "MeshPassProcessor.h"
#include "RenderPass.h"
#include "Guid.h"

namespace Thunder
{
    class RenderTexture;
	class UniformBufferLayout;
	class FrameGraph;
	struct FrameGraphPass;
	struct ShaderParameterMap;
	class ShaderArchive;

    class RenderModule : public IModule
    {
    	DECLARE_MODULE(Render, RenderModule, RENDERCORE_API)
    public:
    	RENDERCORE_API void StartUp() override;
    	RENDERCORE_API void ShutDown() override;

    	RENDERCORE_API static MeshPassProcessor* GetMeshPassProcessor(EMeshPass passType);
    	RENDERCORE_API static void SetMeshPassProcessorCreator(EMeshPass passType, TFunction<class MeshPassProcessor*()> creator);

    	RENDERCORE_API static RenderPass* GetRenderPass(RenderPassKey const& key);

    	// Texture registry.
    	RENDERCORE_API static void RegisterTexture_GameThread(const TGuid& guid, RenderTexture* texture);
    	RENDERCORE_API static void UnregisterTexture_GameThread(const TGuid& guid);
    	RENDERCORE_API static void UpdateTextureRegistry_RenderThread();
    	RENDERCORE_API static RenderTexture* GetTextureResource_RenderThread(const TGuid& guid);

    	// Draw
    	RENDERCORE_API static byte* SetupUniformBufferParameters(const RenderContext* context, const UniformBufferLayout* layout
			, const ShaderParameterMap* parameterMap, const String& ubName);
    	RENDERCORE_API static TRHIGraphicsPipelineState* GetPipelineState(const RenderContext* context, NameHandle subShaderName,
    		ShaderArchive* archive, ShaderCombination* shaderCombination, const SubMesh* subMesh);
    	RENDERCORE_API static void BuildDrawCommand(FrameGraph* graphBuilder, FrameGraphPass* pass, SubMesh* geometry);

    private:
    	std::array<MeshPassProcessorRef, static_cast<size_t>(EMeshPass::Num)> MeshPassProcessors;
    	std::array<TFunction<class MeshPassProcessor*()>, static_cast<size_t>(EMeshPass::Num)> MeshPassProcessorCreators;
    	THashMap<RenderPassKey, RenderPassRef> RenderPasses;

    	// Texture registry.
    	THashMap<TGuid, RenderTexture*> TextureRegistry;
    	THashMap<TGuid, RenderTexture*> TextureRegistrationSet[2];
    	THashSet<TGuid> TextureUnregistrationSet[2];
    };
}
