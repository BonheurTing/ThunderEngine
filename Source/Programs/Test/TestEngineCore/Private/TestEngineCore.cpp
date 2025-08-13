#pragma optimize("", off)

#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	class CoreModule;


	
	int main(int argc, char* argv[])
	{
		Thunder::ModuleManager::GetInstance()->LoadModule<Thunder::CoreModule>();

		IFileSystem* FilsSys = new NativeFileSystem();
		FilsSys->Mount("Content", "");
		{
			TRefCountPtr<NativeFile> file = FilsSys->Open("myfile.bin");
			MemoryWriter archive;
			archive << 5.0f;
			archive << 4u;
			archive << "Hello World!";
			file->Write(archive.Data(), archive.Size());
		}

		{
			TRefCountPtr<NativeFile> file = FilsSys->Open("myfile.bin");
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

		return 0;
	}
}





#pragma optimize("", on)

