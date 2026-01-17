
#include "RenderModule.h"
#include "Misc/CoreGlabal.h"
#include "MeshPassProcessor.h"

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
}
