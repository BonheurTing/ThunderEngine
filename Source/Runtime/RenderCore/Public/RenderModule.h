#pragma once
#include "Container.h"
#include "MeshPass.h"
#include "Platform.h"
#include "Module/ModuleManager.h"
#include <array>
#include "MeshPassProcessor.h"

namespace Thunder
{
    class RENDERCORE_API RenderModule : public IModule
    {
    	DECLARE_MODULE(Render, RenderModule)
    public:
    	void StartUp() override;
    	void ShutDown() override;

    	static MeshPassProcessor* GetMeshPassProcessor(EMeshPass passType);
    	static void SetMeshPassProcessorCreator(EMeshPass passType, TFunction<class MeshPassProcessor*()> creator);

    private:
    	std::array<MeshPassProcessorRef, static_cast<size_t>(EMeshPass::Num)> MeshPassProcessors;
    	std::array<TFunction<class MeshPassProcessor*()>, static_cast<size_t>(EMeshPass::Num)> MeshPassProcessorCreators;
    };
}
