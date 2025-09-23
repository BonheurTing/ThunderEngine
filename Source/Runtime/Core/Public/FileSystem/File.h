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
	
	class CORE_API NativeFile : public IFile
	{
	public:
#ifdef THUNDER_WINDOWS
		NativeFile(HANDLE handle);
#else
		NativeFile(FILE* file);
#endif
		virtual ~NativeFile();

		virtual void Close() override;
		virtual size_t Read(void* buffer, size_t size) override;
		virtual size_t PRead(void* buffer, size_t size, long offset) override;
		virtual BinaryData* ReadData() override;
		virtual String ReadString() override;
		virtual size_t Write(const void* buffer, size_t size) override;
		virtual size_t PWrite(const void* buffer, size_t size, long offset) override;
		virtual size_t Append(const void* buffer, size_t size) override;
		virtual int Seek(long offset, int origin) override;
		virtual long Tell() override;
		virtual bool IsEOF() override;
		virtual bool Flush() override;
		virtual size_t Size() override;
		virtual bool Rename(const String& oldPath, const String& newPath) override;
		
	private:
#ifdef THUNDER_WINDOWS
		HANDLE FileHandle;
#else
		FILE* FileHandle;
#endif
	};

	class CORE_API PackageFile : public IFile
	{
	public:
	private:
		long Size;
		long Offset;
	};
}