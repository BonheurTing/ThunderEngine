#pragma once
#include "Module/ModuleManager.h"
#include "Templates/RefCounting.h"
#include "Config/ConfigManager.h"

namespace Thunder
{
	class IMalloc;
	class FileManager;
	class ConfigManager;
	
	class CORE_API CoreModule : public IModule
	{
		DECLARE_MODULE(Core, CoreModule)
		
	public:
		virtual ~CoreModule() = default;
		void StartUp() override;
		void ShutDown() override;
		

	private:
		IMalloc* MemoryAllocator;
		TRefCountPtr<FileManager> FileManagerInstance;
		TRefCountPtr<ConfigManager> ConfigManagerInstance;
	};

	extern CORE_API FileManager* GFileManager;
	extern CORE_API ConfigManager* GConfigManager;
}