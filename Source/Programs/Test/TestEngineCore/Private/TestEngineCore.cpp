#pragma optimize("", off)

#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "PackageModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/MemoryArchive.h"
#include "Matrix.h"
#include "Assertion.h"

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
	}

	static bool NearlyEqual(float a, float b, float tolerance = 1e-5f)
	{
		return std::fabs(a - b) < tolerance;
	}

	static bool MatrixNearlyEqual(const TMatrix44f& A, const TMatrix44f& B, float tolerance = 1e-5f)
	{
		for (int32 i = 0; i < 4; i++)
			for (int32 j = 0; j < 4; j++)
				if (!NearlyEqual(A.M[i][j], B.M[i][j], tolerance))
					return false;
		return true;
	}
	void TestLoadPackage()
	{
		TArray<GameResource*> res;
		// "/Game/123"
		// "/Game/Mesh/Cube"
		// "/Game/TestPNG"
		//if(PackageModule::ForceLoadPackage("/Game/Mesh/Cube"))
		{
			LOG("success load package, resource count: %llu", res.size());
		}
		//else
		{
			LOG("failed to load package");
		}
	}
}

int main(int argc, char* argv[])
{
	// Thunder::TestMatrix();
	///Thunder::TestFileSystem();
	//Thunder::TestImportResource();
	//Thunder::TestLoadPackage();
	
	Thunder::PackageModule::GetModule()->ImportAll(true);
}

