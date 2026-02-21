
#include "RenderModule.h"

#include "FrameGraph.h"
#include "Misc/CoreGlabal.h"
#include "MeshPassProcessor.h"
#include "RenderTexture.h"
#include "ShaderBindingsLayout.h"
#include "ShaderParameterMap.h"
#include "RenderContext.h"
#include "RenderMesh.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "RHIResource.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Render, RenderModule)

	void RenderModule::StartUp()
	{
		GProceduralGeometryManager = new ProceduralGeometryManager();
		GProceduralGeometryManager->InitBasicGeometrySubMeshes();
	}

	void RenderModule::ShutDown()
	{
		for (auto& processor : MeshPassProcessors)
		{
			processor = nullptr;
		}
		for (auto& creator : MeshPassProcessorCreators)
		{
			creator = nullptr;
		}
		TextureRegistry.clear();
		for (int i = 0; i < 2; ++i)
		{
			TextureRegistrationSet[i].clear();
			TextureUnregistrationSet[i].clear();
		}
		if (GProceduralGeometryManager)
		{
			delete GProceduralGeometryManager;
			GProceduralGeometryManager = nullptr;
		}
	}

	void RenderModule::SetMeshPassProcessorCreator(EMeshPass passType, TFunction<class MeshPassProcessor*()> creator)
	{
		RenderModule* module = GetModule();
		size_t index = static_cast<size_t>(passType);
		if (index < static_cast<size_t>(EMeshPass::Num))
		{
			module->MeshPassProcessorCreators[index] = std::move(creator);
			module->MeshPassProcessors[index] = nullptr; // Reset mesh pass processor.
		}
	}

	RenderPass* RenderModule::GetRenderPass(RenderPassKey const& key)
	{
		// Todo : to be locked.
		auto& renderPassMap = GetModule()->RenderPasses;
		auto passIt = renderPassMap.find(key);
		if (passIt != renderPassMap.end()) [[likely]]
		{
			return passIt->second;
		}

		RenderPass* pass = new RenderPass(key);
		renderPassMap.insert({ key, pass });
		return pass;
	}

	MeshPassProcessor* RenderModule::GetMeshPassProcessor(EMeshPass passType)
	{
		size_t index = static_cast<size_t>(passType);
		if (index >= static_cast<size_t>(EMeshPass::Num))
		{
			return nullptr;
		}

		RenderModule* module = GetModule();
		auto& processor = module->MeshPassProcessors[index];
		if (!processor)
		{
			if (auto& creator = module->MeshPassProcessorCreators[index])
			{
				processor = creator();
			}
			else
			{
				// Default creator.
				processor = new MeshPassProcessor();
			}
		}

		return processor.Get();
	}

	void RenderModule::RegisterTexture_GameThread(const TGuid& guid, RenderTexture* texture)
	{
		uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
		GetModule()->TextureRegistrationSet[gameThreadIndex][guid] = texture;
	}

	void RenderModule::UnregisterTexture_GameThread(const TGuid& guid)
	{
		uint32 const gameThreadIndex = GFrameState->FrameNumberGameThread.load(std::memory_order_acquire) % 2;
		GetModule()->TextureUnregistrationSet[gameThreadIndex].insert(guid);
		GetModule()->TextureRegistrationSet[gameThreadIndex].erase(guid);
	}

	void RenderModule::UpdateTextureRegistry_RenderThread()
	{
		uint32 const renderThreadIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % 2;
		RenderModule* module = GetModule();

		// Register new textures.
		auto const& registerRequests = module->TextureRegistrationSet[renderThreadIndex];
		for (auto const& [guid, texture] : registerRequests)
		{
			module->TextureRegistry[guid] = texture;
		}
		module->TextureRegistrationSet[renderThreadIndex].clear();

		// Unregister removed textures.
		auto const& unregisterRequests = module->TextureUnregistrationSet[renderThreadIndex];
		for (auto const& guid : unregisterRequests)
		{
			module->TextureRegistry.erase(guid);
		}
		module->TextureUnregistrationSet[renderThreadIndex].clear();
	}

	RenderTexture* RenderModule::GetTextureResource_RenderThread(const TGuid& guid)
	{
		auto& registry = GetModule()->TextureRegistry;
		auto it = registry.find(guid);
		if (it != registry.end()) [[likely]]
		{
			return it->second;
		}
		return nullptr;
	}

	byte* RenderModule::SetupUniformBufferParameters(const RenderContext* context, const UniformBufferLayout* layout,
		const ShaderParameterMap* parameterMap, const String& ubName)
	{
		uint32 const bufferSize = layout->GetTotalSize();
		if (bufferSize == 0) [[unlikely]]
		{
			return nullptr;
		}

        // Allocate temporary buffer for packing.
        byte* packedData = static_cast<byte*>(context->Allocate<byte>(bufferSize));
        memset(packedData, 0, bufferSize);

        // Pack parameters into the buffer according to layout.
        auto const& memberMap = layout->GetMemberMap();
        for (auto const& [paramName, memberEntry] : memberMap)
        {
            void const* valuePtr = nullptr;
            uint32 valueSize = 0;

            // Find parameter value based on type.
            switch (memberEntry.Type)
            {
            case EUniformBufferMemberType::Int:
            {
                auto it = parameterMap->IntParameters.find(paramName);
                if (it != parameterMap->IntParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(int32);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in IntParameters for \"%s\".", paramName.c_str(), ubName);
                }
                break;
            }
            case EUniformBufferMemberType::Float:
            {
                auto it = parameterMap->FloatParameters.find(paramName);
                if (it != parameterMap->FloatParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(float);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in FloatParameters for \"%s\".", paramName.c_str(), ubName);
                }
                break;
            }
            case EUniformBufferMemberType::Float4:
            {
                auto it = parameterMap->VectorParameters.find(paramName);
                if (it != parameterMap->VectorParameters.end()) [[likely]]
                {
                    valuePtr = &it->second;
                    valueSize = sizeof(TVector4f);
                }
                else
                {
                    TAssertf(false, "Parameter \"%s\" not found in VectorParameters for \"%s\".", paramName.c_str(), ubName);
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
		return packedData;
	}

	namespace 
	{
		bool GetRenderState(ShaderArchive* archive, NameHandle subShaderName, RHIBlendState& outBlendState, RHIRasterizerState& outRasterizerState, RHIDepthStencilState& outDepthStencilState)
	    {
	        // Get sub-shader.
	        ShaderPass* subShader = archive->GetSubShader(subShaderName);
	        if (!subShader) [[unlikely]]
	        {
	            TAssertf(false, "Sub-shader \"%s\" not found in archive \"%s\".", subShaderName, archive->GetName().c_str());
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

	TRHIGraphicsPipelineState* RenderModule::GetPipelineState(const RenderContext* context, NameHandle subShaderName, ShaderArchive* archive, ShaderCombination* shaderCombination, const SubMesh* subMesh)
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
		bool succeeded = ShaderModule::GetPassRegisterCounts(archive, subShaderName, psoDesc.RegisterCounts);
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
		succeeded = GetRenderState(archive, subShaderName, psoDesc.BlendState, psoDesc.RasterizerState, psoDesc.DepthStencilState);
		if (!succeeded) [[unlikely]]
		{
			TAssertf(false, "Failed to get pipeline state, render state is invalid.");
			return nullptr;
		}

		auto pipelineStateObject = RHICreateGraphicsPipelineState(psoDesc);
		return pipelineStateObject;
	}

	void RenderModule::BuildDrawCommand(FrameGraph* graphBuilder, FrameGraphPass* pass, SubMesh* geometry)
	{
		TAssertf(graphBuilder, "graphBuilder is null");
		auto archive = pass->Archive;
		auto context = graphBuilder->GetMainContext();
		if (context == nullptr) [[unlikely]]
		{
			TAssertf(false, "Context is null");
			return;
		}
		if (pass == nullptr) [[unlikely]]
		{
			TAssertf(false, "CurrentPass is null");
			return;
		}
		if (archive == nullptr)
		{
			TAssertf(false, "Shader is not ready.");
			return;
		}
		NameHandle passName = pass->GetName();

		// update pass uniform buffer
        byte* constantData = SetupUniformBufferParameters(context, pass->PassUBLayout, pass->PassParameters, passName.ToString());
        if (pass->bLayoutNeedsUpdate)
        {
            if (pass->PassUniformBuffer.IsValid())
            {
                RHIDeferredDeleteResource(std::move(pass->PassUniformBuffer));
            }
            pass->PassUniformBuffer = RHICreateUniformBuffer(pass->PassUBLayout->GetTotalSize(), EUniformBufferFlags::UniformBuffer_SingleFrame, constantData);
        }
        else
        {
            RHIUpdateUniformBuffer(context, pass->PassUniformBuffer, constantData);
        }
        pass->bLayoutNeedsUpdate = false;

        // Dispatch command
        RHIDrawCommand* newCommand = new (context->Allocate<RHIDrawCommand>()) RHIDrawCommand;
        uint64 shaderVariantMask = ShaderModule::GetVariantMask(pass->Archive, pass->PassParameters->StaticSwitchParameters);
        newCommand->Shader = ShaderModule::GetShaderCombination(archive, passName, shaderVariantMask);
        newCommand->GraphicsPSO = GetPipelineState(context, passName, archive, newCommand->Shader, geometry);

        // vb ib
        newCommand->VBToSet = geometry->GetVerticesBuffer();
        newCommand->IBToSet = geometry->GetIndicesBuffer();

        // Apply shader bindings.
        newCommand->Bindings.SetTransientAllocated(true);
        SingleShaderBindings* bindings = newCommand->Bindings.GetSingleShaderBindings();
        size_t const bindingSize = pass->BindingLayout->GetTotalSize();
        byte* bindingData = static_cast<byte*>(context->Allocate<byte>(bindingSize));
        memset(bindingData, 0, bindingSize);
        bindings->SetData(bindingData);
        // Bind Constant Buffer
        {
            // Global uniform buffer.
            auto globalUB = graphBuilder->GetGlobalUniformBuffer(); // todo : no need ?
            if (globalUB == nullptr || globalUB->GetResource() == nullptr) [[unlikely]]
            {
                TAssertf(false, "Failed to get global CBV binding : buffer resource is invalid.");
            }

            static NameHandle globalUBName = "Global";
            bindings->SetUniformBuffer(pass->BindingLayout, globalUBName, { .Handle = reinterpret_cast<uint64>(globalUB) });

            // Pass uniform buffer.
            static NameHandle passUBName = "Pass";
            bindings->SetUniformBuffer(pass->BindingLayout, passUBName, { .Handle = reinterpret_cast<uint64>(pass->PassUniformBuffer.Get()) });
        }
        // Bind FGTexture SRV
        {
            auto readFGTextures = pass->GetOperations().GetReadTargets(); // FGTextureID -> Name
            
            for (auto fgTexture : readFGTextures)
            {
                RenderTextureRef rtTexture = graphBuilder->GetAllocatedRenderTarget(fgTexture.first);
                if (!rtTexture.IsValid()) [[unlikely]]
                {
                    TAssertf(false, "Failed to get FGTexture binding : \"%s\", texture resource is invalid.", fgTexture.second.c_str());
                    return;
                }

                // Get resource.
                RHITexture* textureResource = rtTexture->GetTextureRHI();
                if (!textureResource) [[unlikely]]
                {
                    TAssertf(false, "Failed to get SRV binding : \"%s\", texture resource is invalid.", fgTexture.second.c_str());
                    return;
                }

                // Get SRV.
                RHIShaderResourceView* srv = textureResource->GetSRV();
                if (!srv) [[unlikely]]
                {
                    TAssertf(false, "Failed to get SRV binding : \"%s\", SRV is invalid.", fgTexture.second.c_str());
                    return;
                }

                // Bind.
                bindings->SetSRV(pass->BindingLayout, fgTexture.second, { .Handle = srv->GetOfflineHandle() });
            }
            // todo : bind PassParameters->TextureParameters,
        }

        context->AddCommand(newCommand);
	}
}
