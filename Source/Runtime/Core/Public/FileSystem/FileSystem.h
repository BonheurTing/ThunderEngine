#pragma once
#include "Container.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	class IFile;

	class IFileSystem : public RefCountedObject
	{
	public:
		IFileSystem();
		virtual ~IFileSystem();

		virtual IFile* Open(const String& path, const String& mode) = 0;
		virtual bool FileExists(const String& path) = 0;
		virtual bool DirectoryExists(const String& path) = 0;
		virtual bool Delete(const String& path) = 0;
		virtual void Mount(const String& path, const String& mountPoint) = 0;
		virtual void Unmount(const String& mountPoint) = 0;
	};

	class CORE_API NativeFileSystem : public IFileSystem
	{
	public:
		NativeFileSystem();
		virtual ~NativeFileSystem();
	};

	class CORE_API PackageFileSystem : public IFileSystem
	{
	public:
		PackageFileSystem();
		virtual ~PackageFileSystem();
	private:
		
	};
}