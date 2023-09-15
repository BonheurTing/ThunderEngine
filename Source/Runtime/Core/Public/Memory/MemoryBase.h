#pragma once
#include <type_traits>

#include "Platform.h"

namespace Thunder
{
	class CORE_API IMalloc
	{
	public:
		virtual ~IMalloc() {}
		virtual void* Malloc(size_t count, uint32 alignment) = 0;
		virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment) = 0;
		virtual bool GetAllocationSize(void* ptr, size_t& sizeOut) = 0;
		virtual void Free(void* ptr) = 0;
	};
	
	extern CORE_API IMalloc* GMalloc;

	struct TMemory
	{
		template<typename Type>
		static void* Malloc(size_t arrayCount = 1)
		{
			return GMalloc->Malloc(sizeof(Type) * arrayCount, alignof(Type));
		}
		
		template<typename Type>
		static void Destroy(Type*& ptr)
		{
			ptr->~Type();
			GMalloc->Free(ptr);
			ptr = nullptr;
		}
	};
}