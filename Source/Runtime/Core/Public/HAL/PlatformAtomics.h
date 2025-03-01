#pragma once
#include "Platform.h"
#include "Assertion.h"


namespace Thunder
{
	struct FPlatformAtomics
	{
		static FORCEINLINE int8 InterlockedIncrement( volatile int8* Value )
		{
			return (int8)::_InterlockedExchangeAdd8((char*)Value, 1) + 1;
		}

		static FORCEINLINE int16 InterlockedIncrement( volatile int16* Value )
		{
			return (int16)::_InterlockedIncrement16((short*)Value);
		}

		static FORCEINLINE int32 InterlockedIncrement( volatile int32* Value )
		{
			return (int32)::_InterlockedIncrement((long*)Value);
		}

		static FORCEINLINE int64 InterlockedIncrement( volatile int64* Value )
		{
#if PLATFORM_64BITS
			return (int64)::_InterlockedIncrement64((long long*)Value);
#else
			// No explicit instruction for 64-bit atomic increment on 32-bit processors; has to be implemented in terms of CMPXCHG8B
			for (;;)
			{
				int64 OldValue = *Value;
				if (_InterlockedCompareExchange64(Value, OldValue + 1, OldValue) == OldValue)
				{
					return OldValue + 1;
				}
			}
#endif
		}
		


		static FORCEINLINE int8 InterlockedCompareExchange( volatile int8* Dest, int8 Exchange, int8 Comparand )
		{
			return (int8)::_InterlockedCompareExchange8((char*)Dest, (char)Exchange, (char)Comparand);
		}

		static FORCEINLINE int16 InterlockedCompareExchange( volatile int16* Dest, int16 Exchange, int16 Comparand )
		{
			return (int16)::_InterlockedCompareExchange16((short*)Dest, (short)Exchange, (short)Comparand);
		}

		static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
		{
			return (int32)::_InterlockedCompareExchange((long*)Dest, (long)Exchange, (long)Comparand);
		}

		static FORCEINLINE int64 InterlockedCompareExchange( volatile int64* Dest, int64 Exchange, int64 Comparand )
		{
			TAssertf(IsAligned(Dest, alignof(int64)), "InterlockedCompareExchange int64 requires Dest pointer to be aligned to %d bytes", (int)alignof(int64));

			return (int64)::_InterlockedCompareExchange64(Dest, Exchange, Comparand);
		}


		/**
		 * Checks if a pointer is aligned and can be used with atomic functions.
		 *
		 * @param Ptr - The pointer to check.
		 *
		 * @return true if the pointer is aligned, false otherwise.
		 */
		static inline bool IsAligned( const volatile void* Ptr, const uint32 Alignment = sizeof(void*) )
		{
			return !(PTRINT(Ptr) & (Alignment - 1));
		}
		
		static FORCEINLINE int8 AtomicRead(volatile const int8* Src)
		{
			return InterlockedCompareExchange((int8*)Src, 0, 0);
		}

		static FORCEINLINE int16 AtomicRead(volatile const int16* Src)
		{
			return InterlockedCompareExchange((int16*)Src, 0, 0);
		}

		static FORCEINLINE int32 AtomicRead(volatile const int32* Src)
		{
			return InterlockedCompareExchange((int32*)Src, 0, 0);
		}

		static FORCEINLINE int64 AtomicRead(volatile const int64* Src)
		{
			return InterlockedCompareExchange((int64*)Src, 0, 0);
		}

		static FORCEINLINE void* InterlockedCompareExchangePointer( void*volatile* Dest, void* Exchange, void* Comparand )
		{
			TAssertf(IsAligned(Dest, alignof(void*)), "InterlockedCompareExchangePointer requires Dest pointer to be aligned to %d bytes", (int)alignof(void*));
			return ::_InterlockedCompareExchangePointer(Dest, Exchange, Comparand);
		}
	};
}
