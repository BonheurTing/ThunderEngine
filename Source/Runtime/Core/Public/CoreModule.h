#pragma once
#include "Module/ModuleManager.h"

namespace Thunder
{
	class FileManager;
	class IMalloc;
	
	class CORE_API CoreModule : public IModule
	{
		DECLARE_MODULE(Core, CoreModule)
		
	public:
		virtual ~CoreModule() = default;
		void StartUp() override;
		void ShutDown() override {}
		

	private:
		RefCountPtr<FileManager> FileManagerInstance;
		RefCountPtr<IMalloc> MemoryAllocator;
	};

	extern CORE_API FileManager* GFileManager;
}

namespace Thunder
{
/*
	class CORE_API TestModule : public IModule
	{
	public:
		TestModule(NameHandle name) : IModule(name) {}
	};

	class CORE_API TestModule1 : public TestModule
	{
		DECLARE_MODULE_WITH_SUPER(Test1, TestModule1, TestModule)
	public:
		void StartUp() override {}
		void ShutDown() override {}
	};
*/
	
}