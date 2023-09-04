#include "CoreModule.h"

#include "FileManager.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Core, CoreModule)
	FileManager* GFileManager = nullptr;
	
	void CoreModule::StartUp()
	{
		FileManagerInstance = MakeRefCount<FileManager>();
		GFileManager = FileManagerInstance.get();
	}
}

