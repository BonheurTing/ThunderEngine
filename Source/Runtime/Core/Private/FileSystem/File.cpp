#include "FileSystem/File.h"
#include "Memory/MemoryBase.h"
#if THUNDER_WINDOWS
    #include <windows.h>
#elif THUNDER_POSIX
    #include <cstdio>
    #include <unistd.h>
    #include <climits>
    #include <cerrno>
    #include <cstring>
#endif

namespace Thunder
{
#if THUNDER_WINDOWS
	NativeFile::NativeFile(HANDLE handle)
		: FileHandle(handle)
	{
	}
#elif THUNDER_POSIX
	NativeFile::NativeFile(FILE* file)
		: FileHandle(file)
	{
	}
#endif

	NativeFile::~NativeFile()
	{
		Close();
	}

	void NativeFile::Close()
	{
		if (FileHandle)
		{
#if THUNDER_WINDOWS
			CloseHandle(FileHandle);
			FileHandle = nullptr;
#elif THUNDER_POSIX
			fclose(FileHandle);
			FileHandle = nullptr;
#endif
		}
	}

	size_t NativeFile::Read(void* buffer, size_t size)
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		DWORD bytesRead;
		if (!ReadFile(FileHandle, buffer, static_cast<DWORD>(size), &bytesRead, nullptr))
		{
			DWORD error = GetLastError();
			// TODO: Add logging/assertion here
			return 0;
		}
		return static_cast<size_t>(bytesRead);
#elif THUNDER_POSIX
		return fread(buffer, 1, size, FileHandle);
#endif
	}

	size_t NativeFile::PRead(void* buffer, size_t size, long offset)
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		LARGE_INTEGER currentPos;
		if (!SetFilePointerEx(FileHandle, {0}, &currentPos, FILE_CURRENT))
		{
			return 0;
		}
		
		LARGE_INTEGER newPos;
		newPos.QuadPart = offset;
		if (!SetFilePointerEx(FileHandle, newPos, nullptr, FILE_BEGIN))
		{
			return 0;
		}
		
		size_t result = Read(buffer, size);
		
		if (!SetFilePointerEx(FileHandle, currentPos, nullptr, FILE_BEGIN))
		{
			return 0;
		}
		
		return result;
#elif THUNDER_POSIX
		long currentPos = ftell(FileHandle);
		fseek(FileHandle, offset, SEEK_SET);
		size_t result = fread(buffer, 1, size, FileHandle);
		fseek(FileHandle, currentPos, SEEK_SET);
		return result;
#endif
	}

	BinaryData* NativeFile::ReadData()
	{
		if (!FileHandle) return nullptr;
		
#if THUNDER_WINDOWS
		LARGE_INTEGER currentPos;
		if (!SetFilePointerEx(FileHandle, {0}, &currentPos, FILE_CURRENT))
		{
			return nullptr;
		}
		
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(FileHandle, &fileSize))
		{
			return nullptr;
		}
		
		size_t remainingSize = static_cast<size_t>(fileSize.QuadPart - currentPos.QuadPart);
		
		BinaryData* data = new BinaryData();
		data->Size = remainingSize;
		data->Data = TMemory::Malloc<uint8>(data->Size);
		
		size_t bytesRead = Read(data->Data, data->Size);
		if (bytesRead != data->Size)
		{
			TMemory::Free(data->Data);
			delete data;
			return nullptr;
		}
		
		return data;
#elif THUNDER_POSIX
		long currentPos = ftell(FileHandle);
		fseek(FileHandle, 0, SEEK_END);
		long fileSize = ftell(FileHandle);
		fseek(FileHandle, currentPos, SEEK_SET);
		
		BinaryData* data = new BinaryData();
		data->Size = fileSize - currentPos;
		data->Data = TMemory::Malloc<uint8>(data->Size);
		
		fread(data->Data, 1, data->Size, FileHandle);
		return data;
#endif
	}

	String NativeFile::ReadString()
	{
		if (!FileHandle) return String();
		
#if THUNDER_WINDOWS
		LARGE_INTEGER currentPos;
		if (!SetFilePointerEx(FileHandle, {0}, &currentPos, FILE_CURRENT))
		{
			return String();
		}
		
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(FileHandle, &fileSize))
		{
			return String();
		}
		
		size_t remainingSize = static_cast<size_t>(fileSize.QuadPart - currentPos.QuadPart);
		char* buffer = new char[remainingSize + 1];
		
		size_t bytesRead = Read(buffer, remainingSize);
		buffer[bytesRead] = '\0';
		
		String result(buffer);
		delete[] buffer;
		return result;
#elif THUNDER_POSIX
		long currentPos = ftell(FileHandle);
		fseek(FileHandle, 0, SEEK_END);
		long fileSize = ftell(FileHandle);
		fseek(FileHandle, currentPos, SEEK_SET);
		
		size_t remainingSize = fileSize - currentPos;
		char* buffer = new char[remainingSize + 1];
		fread(buffer, 1, remainingSize, FileHandle);
		buffer[remainingSize] = '\0';
		
		String result(buffer);
		delete[] buffer;
		return result;
#endif
	}

	size_t NativeFile::Write(const void* buffer, size_t size)
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		DWORD bytesWritten;
		const bool success = ::WriteFile(FileHandle, buffer, static_cast<DWORD>(size), &bytesWritten, nullptr);
		if (!success || bytesWritten != size)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to write file, total size: %ull, bytes written: %u, errer code: %lu\n", size, bytesWritten, error);
			CloseHandle(FileHandle);
			return 0;
		}
		return static_cast<size_t>(bytesWritten);
#elif THUNDER_POSIX
		return fwrite(buffer, 1, size, FileHandle);
#endif
	}

	size_t NativeFile::PWrite(const void* buffer, size_t size, long offset)
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		LARGE_INTEGER currentPos;
		const bool getCurrentPos = ::SetFilePointerEx(FileHandle, {0}, &currentPos, FILE_CURRENT);
		if (!getCurrentPos)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to get current file position, error code: %lu\n", error);
			return 0;
		}
		
		LARGE_INTEGER newPos;
		newPos.QuadPart = offset;
		const bool setPos = ::SetFilePointerEx(FileHandle, newPos, nullptr, FILE_BEGIN);
		if (!setPos)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to set file position, error code: %lu\n", error);
			return 0;
		}
		
		size_t result = Write(buffer, size);
		
		const bool restorePos = ::SetFilePointerEx(FileHandle, currentPos, nullptr, FILE_BEGIN);
		if (!restorePos)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to restore file position, error code: %lu\n", error);
		}
		
		return result;
#elif THUNDER_POSIX
		long currentPos = ftell(FileHandle);
		fseek(FileHandle, offset, SEEK_SET);
		size_t result = fwrite(buffer, 1, size, FileHandle);
		fseek(FileHandle, currentPos, SEEK_SET);
		return result;
#endif
	}

	size_t NativeFile::Append(const void* buffer, size_t size)
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		const bool setPos = ::SetFilePointerEx(FileHandle, {0}, nullptr, FILE_END);
		if (!setPos)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to set file position to end, error code: %lu\n", error);
			return 0;
		}
		return Write(buffer, size);
#elif THUNDER_POSIX
		fseek(FileHandle, 0, SEEK_END);
		return fwrite(buffer, 1, size, FileHandle);
#endif
	}

	int NativeFile::Seek(long offset, int origin)
	{
		if (!FileHandle) return -1;
#if THUNDER_WINDOWS
		DWORD moveMethod = FILE_BEGIN;
		switch (origin)
		{
		case SEEK_SET: moveMethod = FILE_BEGIN; break;
		case SEEK_CUR: moveMethod = FILE_CURRENT; break;
		case SEEK_END: moveMethod = FILE_END; break;
		default: return -1;
		}
		
		LARGE_INTEGER pos;
		pos.QuadPart = offset;
		const bool success = ::SetFilePointerEx(FileHandle, pos, nullptr, moveMethod);
		if (!success)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to seek file, offset: %ld, origin: %d, error code: %lu\n", offset, origin, error);
			return -1;
		}
		return 0;
#elif THUNDER_POSIX
		return fseek(FileHandle, offset, origin);
#endif
	}

	long NativeFile::Tell()
	{
		if (!FileHandle) return -1;
#if THUNDER_WINDOWS
		LARGE_INTEGER pos;
		const bool success = ::SetFilePointerEx(FileHandle, {0}, &pos, FILE_CURRENT);
		if (!success)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to get file position, error code: %lu\n", error);
			return -1;
		}
		return static_cast<long>(pos.QuadPart);
#elif THUNDER_POSIX
		return ftell(FileHandle);
#endif
	}

	bool NativeFile::IsEOF()
	{
		if (!FileHandle) return true;
#if THUNDER_WINDOWS
		LARGE_INTEGER currentPos;
		const bool getCurrentPos = ::SetFilePointerEx(FileHandle, {0}, &currentPos, FILE_CURRENT);
		if (!getCurrentPos)
		{
			return true;
		}
		
		LARGE_INTEGER fileSize;
		const bool getSize = ::GetFileSizeEx(FileHandle, &fileSize);
		if (!getSize)
		{
			return true;
		}
		
		return currentPos.QuadPart >= fileSize.QuadPart;
#elif THUNDER_POSIX
		return feof(FileHandle) != 0;
#endif
	}

	bool NativeFile::Flush()
	{
		if (!FileHandle) return false;
#if THUNDER_WINDOWS
		const bool success = ::FlushFileBuffers(FileHandle);
		if (!success)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to flush file, error code: %lu\n", error);
			return false;
		}
		return true;
#elif THUNDER_POSIX
		return fflush(FileHandle) == 0;
#endif
	}

	size_t NativeFile::Size()
	{
		if (!FileHandle) return 0;
#if THUNDER_WINDOWS
		LARGE_INTEGER fileSize;
		const bool success = ::GetFileSizeEx(FileHandle, &fileSize);
		if (!success)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to get file size, error code: %lu\n", error);
			return 0;
		}
		return static_cast<size_t>(fileSize.QuadPart);
#elif THUNDER_POSIX
		long currentPos = ftell(FileHandle);
		fseek(FileHandle, 0, SEEK_END);
		long size = ftell(FileHandle);
		fseek(FileHandle, currentPos, SEEK_SET);
		return size;
#endif
	}

	bool NativeFile::Rename(const String& oldPath, const String& newPath)
	{
		if (!FileHandle) return false;
#if THUNDER_WINDOWS
		Close();
		// Perform the rename
		const bool success = ::MoveFileA(oldPath.c_str(), newPath.c_str());
		if (!success)
		{
			const DWORD error = GetLastError();
			TAssertf(false, "Fail to rename file from '%s' to '%s', error code: %lu\n", oldPath, newPath.c_str(), error);
			return false;
		}

		return true;
#elif THUNDER_POSIX
		// For POSIX systems, we need to get the file descriptor and then the path
		int fd = fileno(FileHandle);
		if (fd == -1)
		{
			TAssertf(false, "Fail to get file descriptor\n");
			return false;
		}
		
		// Get current file path using readlink on /proc/self/fd/[fd] (Linux)
		char currentPath[PATH_MAX];
		char fdPath[32];
		snprintf(fdPath, sizeof(fdPath), "/proc/self/fd/%d", fd);
		
		ssize_t pathLen = readlink(fdPath, currentPath, sizeof(currentPath) - 1);
		if (pathLen == -1)
		{
			TAssertf(false, "Fail to get current file path\n");
			return false;
		}
		currentPath[pathLen] = '\0';
		
		// Close the file before renaming
		Close();
		
		// Perform the rename
		const int result = rename(currentPath, newPath.c_str());
		if (result != 0)
		{
			TAssertf(false, "Fail to rename file from '%s' to '%s', errno: %d\n", currentPath, newPath.c_str(), errno);
			return false;
		}
		
		return true;
#endif
	}
}