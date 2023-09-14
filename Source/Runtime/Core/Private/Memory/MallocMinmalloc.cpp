#include "Memory/MallocMinmalloc.h"
#include "mimalloc.h"

namespace Thunder
{
	void* TMallocMinmalloc::Malloc(size_t count)
	{
		//return mi_malloc(count);
		return nullptr;
	}

	void TMallocMinmalloc::Free(void* original)
	{
		//mi_free(original);

	}
}
