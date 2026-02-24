#pragma once
#include "MemoryBase.h"

namespace Thunder
{
	class TMallocMinmalloc : public IMalloc
	{
	public:
		CORE_API void* Malloc(size_t count, uint32 alignment) override;
		CORE_API void* Realloc(void* ptr, size_t newSize, uint32 alignment) override;
		CORE_API bool GetAllocationSize(void* ptr, size_t& sizeOut) override;
		CORE_API void Free( void* original ) override;
	private:
	};
}


