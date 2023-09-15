#include "Memory/MallocAnsi.h"
#include <algorithm>

namespace Thunder
{
	void* TMallocAnsi::Malloc(size_t count, uint32 alignment)
	{
		alignment = std::max(count >= 16 ? static_cast<uint32>(16) : static_cast<uint32>(8), alignment);
		void* result = _aligned_malloc(count, alignment);
		return result;
	}

	void* TMallocAnsi::Realloc(void* ptr, size_t newSize, uint32 alignment)
	{
		alignment = std::max(newSize >= 16 ? static_cast<uint32>(16) : static_cast<uint32>(8), alignment);
		void* result = _aligned_realloc(ptr, newSize, alignment);
		return result;
	}

	bool TMallocAnsi::GetAllocationSize(void* ptr, size_t& sizeOut)
	{
		if (!ptr)
		{
			return false;
		}
		sizeOut = _aligned_msize(ptr, 16, 0);
		return true;
	}

	void TMallocAnsi::Free(void* original)
	{
		_aligned_free(original);
	}
}
