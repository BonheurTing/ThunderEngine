#pragma once
#include <atomic>
#include "Assertion.h"

namespace Thunder
{
	class CORE_API RefCountedObject
	{
		//typename NeedAutoMemoryFree true;
		public:
		RefCountedObject(): NumRefs(0) {}
		virtual ~RefCountedObject()
		{
			TAssert(!NumRefs);
		}
		RefCountedObject(const RefCountedObject& Rhs) = delete;
		RefCountedObject& operator=(const RefCountedObject& Rhs) = delete;
		// ReSharper disable once CppMemberFunctionMayBeStatic
		bool NeedAutoMemoryFree() const { return true; }
		uint32 AddRef() const
		{
			return NumRefs.fetch_add(1, std::memory_order_acquire);
		}
		int Release(bool isManaged) const
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
		uint32 GetRefCount() const
		{
			return NumRefs.load(std::memory_order_acquire);
		}
		
	private:
		mutable std::atomic_int NumRefs;
	};
}