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

	class CORE_API NativeFileSystem : public IFileSystem
	{
	public:
		NativeFileSystem();
		virtual ~NativeFileSystem();

		virtual IFile* CreateFile(const String& path) override;
		virtual IFile* Open(const String& path, bool bNeedJoin = false) override;
		virtual bool FileExists(const String& path) override;
		virtual bool DirectoryExists(const String& path) override;
		virtual bool Delete(const String& path) override;
		virtual void Mount(const String& path) override;
		virtual void Unmount(const String& mountPoint) override;

	private:
		String BasePath;
	};

	class CORE_API PackageFileSystem : public IFileSystem
	{
	public:
		PackageFileSystem();
		virtual ~PackageFileSystem();
	private:
		
	};
}
