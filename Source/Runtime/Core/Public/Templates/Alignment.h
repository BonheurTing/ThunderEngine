#pragma once
#include "Container.h"
#include "Platform.h"

namespace Thunder
{
	/**
	 * Checks if a pointer is aligned to the specified alignment.
	 *
	 * @param  Val        The value to align.
	 * @param  Alignment  The alignment value, must be a power of two.
	 *
	 * @return true if the pointer is aligned to the specified alignment, false otherwise.
	 */
	template <typename T>
	FORCEINLINE constexpr bool IsAligned(T Val, uint64 Alignment)
	{
		//todo: static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "IsAligned expects an integer or pointer type");

		return !((uint64)Val & (Alignment - 1));
	}
}
