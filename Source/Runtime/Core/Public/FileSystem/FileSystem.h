#pragma once
#include "Container.h"
#include "Templates/RefCounting.h"
#include <cstdio>

namespace Thunder
{
	class IFile;

	class IFileSystem : public RefCountedObject
	{
	public:
		IFileSystem();
		virtual ~IFileSystem();

		virtual IFile* CreateFile(const String& path) = 0;
		virtual IFile* Open(const String& path, bool bNeedJoin = false) = 0;
		virtual bool FileExists(const String& path) = 0;
		virtual bool DirectoryExists(const String& path) = 0;
		virtual bool Delete(const String& path) = 0;
		virtual void Mount(const String& path) = 0;
		virtual void Unmount(const String& mountPoint) = 0;
	};
	using IFileSystemRef = TRefCountPtr<IFileSystem>;

	class NativeFileSystem : public IFileSystem
	{
	public:
		CORE_API NativeFileSystem();
		CORE_API virtual ~NativeFileSystem();

		CORE_API virtual IFile* CreateFile(const String& path) override;
		CORE_API virtual IFile* Open(const String& path, bool bNeedJoin = false) override;
		CORE_API virtual bool FileExists(const String& path) override;
		CORE_API virtual bool DirectoryExists(const String& path) override;
		CORE_API virtual bool Delete(const String& path) override;
		CORE_API virtual void Mount(const String& path) override;
		CORE_API virtual void Unmount(const String& mountPoint) override;

	private:
		String BasePath;
	};

	class PackageFileSystem : public IFileSystem
	{
	public:
		CORE_API PackageFileSystem();
		CORE_API virtual ~PackageFileSystem();
	private:
		
	};
}
