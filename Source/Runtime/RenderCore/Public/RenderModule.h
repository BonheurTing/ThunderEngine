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

    class RENDERCORE_API RenderModule : public IModule
    {
    	DECLARE_MODULE(Render, RenderModule)
    public:
    	void StartUp() override;
    	void ShutDown() override;

    	static MeshPassProcessor* GetMeshPassProcessor(EMeshPass passType);
    	static void SetMeshPassProcessorCreator(EMeshPass passType, TFunction<class MeshPassProcessor*()> creator);

    	static RenderPass* GetRenderPass(RenderPassKey const& key);

    	// Texture registry.
    	static void RegisterTexture_GameThread(const TGuid& guid, RenderTexture* texture);
    	static void UnregisterTexture_GameThread(const TGuid& guid);
    	static void UpdateTextureRegistry_RenderThread();
    	static RenderTexture* GetTextureResource_RenderThread(const TGuid& guid);

    	// Draw
    	static byte* SetupUniformBufferParameters(const RenderContext* context, const UniformBufferLayout* layout
			, const ShaderParameterMap* parameterMap, const String& ubName);
    	static TRHIGraphicsPipelineState* GetPipelineState(const RenderContext* context, NameHandle subShaderName,
    		ShaderArchive* archive, ShaderCombination* shaderCombination, const SubMesh* subMesh);
    	static void BuildDrawCommand(FrameGraph* graphBuilder, FrameGraphPass* pass, SubMesh* geometry);

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
