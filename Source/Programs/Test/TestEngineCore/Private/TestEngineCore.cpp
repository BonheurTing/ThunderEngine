#pragma optimize("", off)

#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "ResourceModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	class CoreModule;

	void TestFileSystem()
	{
		ModuleManager::GetInstance()->LoadModule<CoreModule>();
		ModuleManager::GetInstance()->LoadModule<FileModule>();

		IFileSystem* FileSys = FileModule::GetFileSystem("Content");
		{
			TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(FileSys->Open("myfile.bin"));
			MemoryWriter archive;
			archive << 5.0f;
			archive << 4u;
			archive << "Hello World!";
			file->Write(archive.Data(), archive.Size());
		}

		{
			TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(FileSys->Open("myfile.bin"));
			TRefCountPtr<BinaryData> data = file->ReadData();
			MemoryReader archive(data.Get());
			float a;
			int b;
			String c;
			archive >> a;
			archive >> b;
			archive >> c;
			std::cout << a << ";" << b << ";" << c << std::endl;
		}

	}

	void TestImportResource()
	{
		ResourceModule::ImportAll();
	}
}

int main(int argc, char* argv[])
{
	Thunder::TestFileSystem();
	Thunder::TestImportResource();
}




#pragma optimize("", on)

