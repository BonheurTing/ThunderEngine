#pragma once
#include "Module/ModuleManager.h"
#include "Templates/RefCounting.h"
#include "Config/ConfigManager.h"

namespace Thunder
{
	
	class CoreModule : public IModule
	{
		DECLARE_MODULE(Core, CoreModule, CORE_API)
		
	public:
		CORE_API ~CoreModule() override = default;
		CORE_API void StartUp() override;
		CORE_API void ShutDown() override;

	private:
		class IMalloc* MemoryAllocator;
		TRefCountPtr<ConfigManager> ConfigManagerInstance;
	};

	extern CORE_API ConfigManager* GConfigManager;
}