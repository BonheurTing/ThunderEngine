#include "CoreModule.h"
#include "FileManager.h"
#include "Memory/MallocMinmalloc.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Core, CoreModule)
	FileManager* GFileManager = nullptr;
	
	void CoreModule::StartUp()
	{
		FileManagerInstance = MakeRefCount<FileManager>();
		GFileManager = FileManagerInstance.get();

		MemoryAllocator = MakeRefCount<TMallocMinmalloc>();
		GMalloc = MemoryAllocator.get();
	}
}

namespace Thunder
{
	//IMPLEMENT_MODULE(Test1, TestModule1)
}