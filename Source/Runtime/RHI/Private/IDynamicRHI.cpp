#include "IDynamicRHI.h"

namespace Thunder
{
	IDynamicRHI* GDynamicRHI = nullptr;
	std::atomic_uint64_t GCachedMeshDrawCommandIndex = 0;
}
