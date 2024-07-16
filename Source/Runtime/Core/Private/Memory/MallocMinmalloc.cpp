#include "Memory/MallocMinmalloc.h"
#include "mimalloc.h"

namespace Thunder
{
void* TMallocMinmalloc::Malloc(size_t count, uint32 alignment)
{
	void* result = nullptr;
	if (alignment != 0)
	{
		alignment = std::max(static_cast<uint32>(count >= 16 ? 16 : 8), alignment);
		result = mi_malloc_aligned(count, alignment); //
	}
	else
	{
		result = mi_malloc_aligned(count, static_cast<uint32>(count >= 16 ? 16 : 8));
	}
	return result;
}

void* TMallocMinmalloc::Realloc(void* ptr, size_t newSize, uint32 alignment)
{
	if (newSize == 0)
	{
		mi_free(ptr);
		return nullptr;
	}
	void* result = nullptr;
	if (alignment != 0)
	{
		alignment = std::max(newSize >= 16 ? static_cast<uint32>(16) : static_cast<uint32>(8), alignment);
		result = mi_realloc_aligned(ptr, newSize, alignment);
	}
	else
	{
		result = mi_realloc(ptr, newSize);
	}
	return result;
}

bool TMallocMinmalloc::GetAllocationSize(void* ptr, size_t& sizeOut)
{
	sizeOut = mi_malloc_size(ptr);
	return true;
}

void TMallocMinmalloc::Free(void* original)
{
	if (!original)
	{
		return;
	}
	mi_free(original);
}
}
