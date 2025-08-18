#pragma once
#include "BasicDefinition.h"
#include "Container.h"
#include <cstring>

#include "Guid.h"

namespace Thunder
{
	class CORE_API MemoryWriter
	{
	public:
		MemoryWriter();
		~MemoryWriter();

		template<typename T>
		MemoryWriter& operator<<(const T& value);

		MemoryWriter& operator<<(const String& str);
		MemoryWriter& operator<<(const char* str);
		MemoryWriter& operator<<(const TGuid& guid)
		{
			return *this << guid.A << guid.B << guid.C << guid.D;
		} 
		

		void* Data() const { return Buffer; }
		size_t Size() const { return CurrentSize; }

	private:
		void EnsureCapacity(size_t additionalSize);
		void WriteRaw(const void* data, size_t size);

		void* Buffer;
		size_t CurrentSize;
		size_t Capacity;
	};

	class CORE_API MemoryReader
	{
	public:
		MemoryReader(BinaryData* data);
		~MemoryReader();

		template<typename T>
		MemoryReader& operator>>(T& value);

		MemoryReader& operator>>(String& str);
		MemoryReader& operator>>(const char* str);
		MemoryReader& operator>>(TGuid& guid)
		{
			return *this >> guid.A >> guid.B >> guid.C >> guid.D;
		}

	private:
		void ReadRaw(void* dest, size_t size);

		const void* Buffer;
		size_t BufferSize;
		size_t Position;
	};

	template<typename T>
	MemoryWriter& MemoryWriter::operator<<(const T& value)
	{
		WriteRaw(&value, sizeof(T));
		return *this;
	}

	template<typename T>
	MemoryReader& MemoryReader::operator>>(T& value)
	{
		ReadRaw(&value, sizeof(T));
		return *this;
	}
}
