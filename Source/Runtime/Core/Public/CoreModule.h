#pragma once
#include "Module/ModuleManager.h"

namespace Thunder
{
	class FileManager;
	
	class CORE_API CoreModule : public IModule
	{
		DECLARE_MODULE(Core, CoreModule)
	public:
		virtual ~CoreModule() = default;
		void StartUp() override;
		void ShutDown() override {}
		

	private:
		RefCountPtr<FileManager> FileManagerInstance;
		
	};

	extern CORE_API FileManager* GFileManager;
}

