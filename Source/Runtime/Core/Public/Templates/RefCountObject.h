#pragma once
#include <atomic>
#include "Assertion.h"

namespace Thunder
{
	class RefCountedObject
	{
		//typename NeedAutoMemoryFree true;
	public:
		CORE_API RefCountedObject(): NumRefs(0) {}
		CORE_API virtual ~RefCountedObject()
		{
			TAssert(!NumRefs);
		}
		CORE_API RefCountedObject(const RefCountedObject& Rhs) = delete;
		CORE_API RefCountedObject& operator=(const RefCountedObject& Rhs) = delete;
		// ReSharper disable once CppMemberFunctionMayBeStatic
		CORE_API bool NeedAutoMemoryFree() const { return true; }
		CORE_API uint32 AddRef() const
		{
			return NumRefs.fetch_add(1, std::memory_order_acquire);
		}
		CORE_API int Release(bool isManaged) const
		{
			const int oldRefCount = NumRefs.fetch_sub(1, std::memory_order_release);
			TAssertf(oldRefCount > 0, "Reference is released more than once.");
			if (oldRefCount == 1)
			{
				if (!isManaged)
				{
					delete this;
				}
			}
			return oldRefCount - 1;
		}
		CORE_API uint32 GetRefCount() const
		{
			return NumRefs.load(std::memory_order_acquire);
		}
		
	private:
		mutable std::atomic_int NumRefs;
	};
}