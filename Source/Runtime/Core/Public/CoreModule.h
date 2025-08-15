#pragma once
#include "Module/ModuleManager.h"
#include "Templates/RefCounting.h"
#include "Config/ConfigManager.h"

namespace Thunder
{
	
	class CORE_API CoreModule : public IModule
	{
		DECLARE_MODULE(Core, CoreModule)
		
	public:
		~CoreModule() override = default;
		void StartUp() override;
		void ShutDown() override;

	private:
		class IMalloc* MemoryAllocator;
		TRefCountPtr<ConfigManager> ConfigManagerInstance;
	};

	extern CORE_API ConfigManager* GConfigManager;
}