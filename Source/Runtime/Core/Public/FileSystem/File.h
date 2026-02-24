#pragma once
#include "BinaryData.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	class IFile : public RefCountedObject
	{
	public:
		IFile() {}
		virtual ~IFile() {}

		virtual void Close() = 0;
		virtual size_t Read(void* buffer, size_t size) = 0;
		virtual size_t PRead(void* buffer, size_t size, long offset) = 0;
		virtual BinaryData* ReadData() = 0;
		virtual String ReadString() = 0;
		virtual size_t Write(const void* buffer, size_t size) = 0;
		virtual size_t PWrite(const void* buffer, size_t size, long offset) = 0;
		virtual size_t Append(const void* buffer, size_t size) = 0;
		virtual int Seek(long offset, int origin) = 0;
		virtual long Tell() = 0;
		virtual bool IsEOF() = 0;
		virtual bool Flush() = 0;
		virtual size_t Size() = 0;
		virtual bool Rename(const String& oldPath, const String& newPath) = 0;
	};
	using IFileRef = TRefCountPtr<IFile>;
	
	class NativeFile : public IFile
	{
	public:
#ifdef THUNDER_WINDOWS
		CORE_API NativeFile(HANDLE handle);
#else
		CORE_API NativeFile(FILE* file);
#endif
		CORE_API virtual ~NativeFile();

		CORE_API virtual void Close() override;
		CORE_API virtual size_t Read(void* buffer, size_t size) override;
		CORE_API virtual size_t PRead(void* buffer, size_t size, long offset) override;
		CORE_API virtual BinaryData* ReadData() override;
		CORE_API virtual String ReadString() override;
		CORE_API virtual size_t Write(const void* buffer, size_t size) override;
		CORE_API virtual size_t PWrite(const void* buffer, size_t size, long offset) override;
		CORE_API virtual size_t Append(const void* buffer, size_t size) override;
		CORE_API virtual int Seek(long offset, int origin) override;
		CORE_API virtual long Tell() override;
		CORE_API virtual bool IsEOF() override;
		CORE_API virtual bool Flush() override;
		CORE_API virtual size_t Size() override;
		CORE_API virtual bool Rename(const String& oldPath, const String& newPath) override;
		
	private:
#ifdef THUNDER_WINDOWS
		HANDLE FileHandle;
#else
		FILE* FileHandle;
#endif
	};

	class PackageFile : public IFile
	{
	public:
	private:
		long Size;
		long Offset;
	};
}