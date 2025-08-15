#include "CoreModule.h"
#include "Memory/MallocAnsi.h"
#include "Memory/MallocMinmalloc.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Core, CoreModule)
	ConfigManager* GConfigManager = nullptr;
	
	void CoreModule::StartUp()
	{
		MemoryAllocator = new TMallocMinmalloc(); //
		GMalloc = MemoryAllocator;

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