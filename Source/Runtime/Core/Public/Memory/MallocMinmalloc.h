#pragma once
#include "MemoryBase.h"

namespace Thunder
{
	class CORE_API TMallocMinmalloc : public IMalloc
	{
	public:
		void* Malloc(size_t count) override;
		void* TryMalloc( size_t count) override { return nullptr; }
		void Free( void* original ) override;
	private:
	};
}


