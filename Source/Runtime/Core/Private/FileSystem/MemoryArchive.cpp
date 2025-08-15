#include "FileSystem/MemoryArchive.h"
#include "Memory/MemoryBase.h"

namespace Thunder
{
	MemoryWriter::MemoryWriter()
		: Buffer(nullptr)
		, CurrentSize(0)
		, Capacity(0)
	{
		EnsureCapacity(1024);
	}

	MemoryWriter::~MemoryWriter()
	{
		if (Buffer)
		{
			TMemory::Destroy(Buffer);
		}
	}

	MemoryWriter& MemoryWriter::operator<<(const String& str)
	{
		size_t length = str.length();
		WriteRaw(&length, sizeof(size_t));
		WriteRaw(str.data(), length);
		return *this;
	}

	MemoryWriter& MemoryWriter::operator<<(const char* str)
	{
		return *this << String(str);
	}

	void MemoryWriter::EnsureCapacity(size_t additionalSize)
	{
		if (CurrentSize + additionalSize > Capacity)
		{
			size_t newCapacity = (Capacity == 0) ? 1024 : Capacity * 2;
			while (newCapacity < CurrentSize + additionalSize)
			{
				newCapacity *= 2;
			}

			void* newBuffer = TMemory::Malloc<uint8>(newCapacity);
			if (Buffer)
			{
				memcpy(newBuffer, Buffer, CurrentSize);
				TMemory::Destroy(Buffer);
			}
			Buffer = newBuffer;
			Capacity = newCapacity;
		}
	}

	void MemoryWriter::WriteRaw(const void* data, size_t size)
	{
		EnsureCapacity(size);
		memcpy(static_cast<uint8*>(Buffer) + CurrentSize, data, size);
		CurrentSize += size;
	}

	MemoryReader::MemoryReader(BinaryData* data)
		: Buffer(data->Data)
		, BufferSize(data->Size)
		, Position(0)
	{
	}

	MemoryReader::~MemoryReader()
	{
	}

	MemoryReader& MemoryReader::operator>>(String& str)
	{
		size_t length;
		ReadRaw(&length, sizeof(size_t));
		
		char* temp = new char[length + 1];
		ReadRaw(temp, length);
		temp[length] = '\0';
		str = String(temp);
		delete[] temp;
		
		return *this;
	}

	void MemoryReader::ReadRaw(void* dest, size_t size)
	{
		if (Position + size <= BufferSize)
		{
			memcpy(dest, static_cast<const uint8*>(Buffer) + Position, size);
			Position += size;
		}
	}
}