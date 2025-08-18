#include "FileSystem/FileSystem.h"
#include "FileSystem/File.h"
#if THUNDER_WINDOWS
    #include <windows.h>
#elif THUNDER_POSIX
    #include <sys/stat.h>
#endif

namespace Thunder
{
	IFileSystem::IFileSystem()
	{
	}

	IFileSystem::~IFileSystem()
	{
	}

	NativeFileSystem::NativeFileSystem()
		: BasePath("")
	{
	}

	NativeFileSystem::~NativeFileSystem()
	{
	}

	IFile* NativeFileSystem::CreateFile(const String& path)
	{
		return Open(path);
	}

	IFile* NativeFileSystem::Open(const String& path, bool bFullPath)
	{
		String fullPath = bFullPath || BasePath.empty()? path : BasePath + path;
#if THUNDER_WINDOWS
		HANDLE handle = CreateFileA(fullPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
									nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		
		if (handle != INVALID_HANDLE_VALUE)
		{
			return new NativeFile(handle);
		}
		return nullptr;
#elif THUNDER_POSIX
		FILE* file = fopen(fullPath.c_str(), "rb+");
		
		if (!file)
		{
			file = fopen(fullPath.c_str(), "wb+");
		}
		
		if (file)
		{
			return new NativeFile(file);
		}
		return nullptr;
#endif
	}

	bool NativeFileSystem::FileExists(const String& path)
	{
		String fullPath = BasePath.empty() ? path : BasePath + "/" + path;
#if THUNDER_WINDOWS
		DWORD attributes = GetFileAttributesA(fullPath.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
#elif THUNDER_POSIX
		struct stat buffer;
		return (stat(fullPath.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
	}

	bool NativeFileSystem::DirectoryExists(const String& path)
	{
		String fullPath = BasePath.empty() ? path : BasePath + "/" + path;
#if THUNDER_WINDOWS
		DWORD attributes = GetFileAttributesA(fullPath.c_str());
		return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#elif THUNDER_POSIX
		struct stat buffer;
		return (stat(fullPath.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
#endif
	}

	bool NativeFileSystem::Delete(const String& path)
	{
		String fullPath = BasePath.empty() ? path : BasePath + "/" + path;
		return remove(fullPath.c_str()) == 0;
	}

	void NativeFileSystem::Mount(const String& path)
	{
		BasePath = path;
	}

	void NativeFileSystem::Unmount(const String& mountPoint)
	{
		BasePath = "";
	}

	PackageFileSystem::PackageFileSystem()
	{
	}

	PackageFileSystem::~PackageFileSystem()
	{
	}
}