#pragma optimize("", off)

#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "PackageModule.h"
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
		ModuleManager::GetInstance()->LoadModule<PackageModule>();

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
			BinaryDataRef data = file->ReadData();
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
		
		PackageModule::GetModule()->ImportAll(true);
		/*const String fileName = FileModule::GetProjectRoot() + "\\Resource\\Mesh\\Cube.fbx";
		const String destPath = FileModule::GetProjectRoot() + "\\Content\\Mesh\\Cube.tasset";*/
		/*const String fileName = FileModule::GetProjectRoot() + "\\Resource\\TestPNG.png";
		const String destPath = FileModule::GetProjectRoot() + "\\Content\\TestPNG.tasset";
		ResourceModule::Import(fileName, destPath);*/
	}

	void TestLoadPackage()
	{
		TArray<GameResource*> res;
		// "/Game/123"
		// "/Game/Mesh/Cube"
		// "/Game/TestPNG"
		if(PackageModule::ForceLoadPackage("/Game/Mesh/Cube"))
		{
			LOG("success load package, resource count: %llu", res.size());
		}
		else
		{
			LOG("failed to load package");
		}
	}
}

int main(int argc, char* argv[])
{
	Thunder::TestFileSystem();
	Thunder::TestImportResource();
	Thunder::TestLoadPackage();
}

