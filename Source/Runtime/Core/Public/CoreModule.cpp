#include "CoreModule.h"
#include "Memory/MallocAnsi.h"
#include "Memory/MallocMinmalloc.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Core, CoreModule)
	FileManager* GFileManager = nullptr;
	ConfigManager* GConfigManager = nullptr;
	
	void CoreModule::StartUp()
	{
		MemoryAllocator = new TMallocMinmalloc(); //
		GMalloc = MemoryAllocator;

		FileManagerInstance = MakeRefCount<FileManager>();
		GFileManager = FileManagerInstance.Get();

		ConfigManagerInstance = MakeRefCount<ConfigManager>();
		GConfigManager = ConfigManagerInstance.Get();
	}

	void CoreModule::ShutDown()
	{
		if (MemoryAllocator)
		{
			delete MemoryAllocator;
			MemoryAllocator = nullptr;
			GMalloc = nullptr;
		}
	}
}