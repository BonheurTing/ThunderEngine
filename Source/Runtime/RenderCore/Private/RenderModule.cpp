
#include "RenderModule.h"

#include "IDynamicRHI.h"
#include "Misc/CoreGlabal.h"
#include "MeshPassProcessor.h"
#include "RenderTexture.h"
#include "ShaderBindingsLayout.h"
#include "ShaderParameterMap.h"
#include "RenderContext.h"
#include "RHIResource.h"
#include "RenderContext.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Render, RenderModule)

	void RenderModule::StartUp()
	{
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

	class MeshPassProcessor* RenderModule::GetMeshPassProcessor(EMeshPass passType)
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

	void RenderModule::PackUniformBuffer(const RenderContext* context, const UniformBufferLayout* layout,
		const ShaderParameterMap* parameterMap, RHIConstantBufferRef& uniformBuffer, bool cacheMeshDrawCommand, const String& ubName)
	{
		uint32 const bufferSize = layout->GetTotalSize();
		if (bufferSize == 0) [[unlikely]]
		{
			return;
		}

		// Create or update RHI constant buffer.
		// Todo : dynamic draw commands should use a transient allocator.
		if (!uniformBuffer)
		{
			// Create new constant buffer.
			uniformBuffer = RHICreateConstantBuffer(bufferSize, EBufferCreateFlags::Dynamic); // Maybe on default heap?
			if (!uniformBuffer) [[unlikely]]
			{
				TAssertf(false, "Failed to create constant buffer.");
				return;
			}

			// Create constant buffer view.
			RHICreateConstantBufferView(*uniformBuffer.Get(), bufferSize);
		}

        // Allocate temporary buffer for packing.
        byte* packedData = (cacheMeshDrawCommand) ?
            static_cast<byte*>(TMemory::Malloc(bufferSize, 16)) :
            static_cast<byte*>(context->Allocate<byte>(bufferSize));
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
}
