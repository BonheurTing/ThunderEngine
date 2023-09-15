#pragma once
#include "MemoryBase.h"

namespace Thunder
{
	class TMallocAnsi : public IMalloc
	{
	public:
		void* Malloc(size_t count, uint32 alignment) override;
		void* Realloc(void* ptr, size_t newSize, uint32 alignment) override;
		bool GetAllocationSize(void* ptr, size_t& sizeOut) override;
		void Free( void* original ) override;
	private:
	
	};
}
