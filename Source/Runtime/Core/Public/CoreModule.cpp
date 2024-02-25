#include "CoreModule.h"
#include "FileManager.h"
#include "Config/ConfigManager.h"
#include "Memory/MallocMinmalloc.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Core, CoreModule)
	FileManager* GFileManager = nullptr;
	ConfigManager* GConfigManager = nullptr;
	
	void CoreModule::StartUp()
	{
		MemoryAllocator = MakeRefCount<TMallocMinmalloc>();
		GMalloc = MemoryAllocator.get();

		FileManagerInstance = MakeRefCount<FileManager>();
		GFileManager = FileManagerInstance.get();

		ConfigManagerInstance = MakeRefCount<ConfigManager>();
		GConfigManager = ConfigManagerInstance.get();
	}
}

namespace Thunder
{
	//IMPLEMENT_MODULE(Test1, TestModule1)
}