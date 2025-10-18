#include "Container/LockFree.h"
#pragma optimize("", off)
namespace Thunder
{

class LockFreeLinkAllocator_TLSCache
{
	enum
	{
		NUM_PER_BUNDLE = 64,
	};

	typedef LockFreeLinkPolicy::TLink TLink;
	typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;

public:

	LockFreeLinkAllocator_TLSCache()
	{
		//check(IsInGameThread());
		TlsSlot = FPlatformTLS::AllocTlsSlot();
		TAssert(FPlatformTLS::IsValidTlsSlot(TlsSlot));
	}
	/** Destructor, leaks all of the memory **/
	~LockFreeLinkAllocator_TLSCache()
	{
		FPlatformTLS::FreeTlsSlot(TlsSlot);
		TlsSlot = 0;
	}

	/**
	* Allocates a memory block of size SIZE.
	*
	* @return Pointer to the allocated memory.
	* @see Free
	*/
	TLinkPtr Pop()
	{
		FThreadLocalCache& tls = GetTLS();

		if (!tls.PartialBundle)
		{
			if (tls.FullBundle)
			{
				tls.PartialBundle = tls.FullBundle;
				tls.FullBundle = 0;
			}
			else
			{
				tls.PartialBundle = GlobalFreeListBundles.Pop();
				if (!tls.PartialBundle)
				{
					int32 firstIndex = LockFreeLinkPolicy::LinkAllocator.Alloc(NUM_PER_BUNDLE);
					for (int32 Index = 0; Index < NUM_PER_BUNDLE; Index++)
					{
						TLink* event = LockFreeLinkPolicy::IndexToLink(firstIndex + Index);
						event->DoubleNext.Init();
						event->SingleNext = 0;
						event->Payload = (void*)UPTRINT(tls.PartialBundle);
						tls.PartialBundle = LockFreeLinkPolicy::IndexToPtr(firstIndex + Index);
					}
				}
			}
			tls.NumPartial = NUM_PER_BUNDLE;
		}
		TLinkPtr result = tls.PartialBundle;
		TLink* resultP = LockFreeLinkPolicy::DerefLink(tls.PartialBundle);
		tls.PartialBundle = TLinkPtr(UPTRINT(resultP->Payload));
		tls.NumPartial--;
		//checkLockFreePointerList(tls.NumPartial >= 0 && ((!!tls.NumPartial) == (!!tls.PartialBundle)));
		resultP->Payload = nullptr;
		checkLockFreePointerList(!resultP->DoubleNext.GetPtr() && !resultP->SingleNext);
		return result;
	}

	/**
	* Puts a memory block previously obtained from Allocate() back on the free list for future use.
	*
	* @param Item The item to free.
	* @see Allocate
	*/
	void Push(TLinkPtr Item)
	{
		FThreadLocalCache& tls = GetTLS();
		if (tls.NumPartial >= NUM_PER_BUNDLE)
		{
			if (tls.FullBundle)
			{
				GlobalFreeListBundles.Push(tls.FullBundle);
				//tls.FullBundle = nullptr;
			}
			tls.FullBundle = tls.PartialBundle;
			tls.PartialBundle = 0;
			tls.NumPartial = 0;
		}
		TLink* itemP = LockFreeLinkPolicy::DerefLink(Item);
		itemP->DoubleNext.SetPtr(0);
		itemP->SingleNext = 0;
		itemP->Payload = (void*)UPTRINT(tls.PartialBundle);
		tls.PartialBundle = Item;
		tls.NumPartial++;
	}

private:

	/** struct for the TLS cache. */
	struct FThreadLocalCache
	{
		TLinkPtr FullBundle;
		TLinkPtr PartialBundle;
		int32 NumPartial;

		FThreadLocalCache()
			: FullBundle(0)
			, PartialBundle(0)
			, NumPartial(0)
		{
		}
	};

	FThreadLocalCache& GetTLS()
	{
		TAssert(FPlatformTLS::IsValidTlsSlot(TlsSlot));
		FThreadLocalCache* tls = (FThreadLocalCache*)FPlatformTLS::GetTlsValue(TlsSlot);
		if (!tls)
		{
			tls = new FThreadLocalCache();
			FPlatformTLS::SetTlsValue(TlsSlot, tls);
		}
		return *tls;
	}

	/** Slot for TLS struct. */
	uint32 TlsSlot;

	/** Lock free list of free memory blocks, these are all linked into a bundle of NUM_PER_BUNDLE. */
	LockFreePointerListLIFORoot<PLATFORM_CACHE_LINE_SIZE> GlobalFreeListBundles;
};

static LockFreeLinkAllocator_TLSCache& GetLockFreeAllocator()
{
	// make memory that will not go away, a replacement for TLazySingleton, which will still get destructed
	alignas(LockFreeLinkAllocator_TLSCache) static unsigned char Data[sizeof(LockFreeLinkAllocator_TLSCache)];
	static bool bIsInitialized = false;
	if (!bIsInitialized)
	{
		new(Data)LockFreeLinkAllocator_TLSCache();
		bIsInitialized = true;
	}
	return *(LockFreeLinkAllocator_TLSCache*)Data;
}

void LockFreeLinkPolicy::FreeLockFreeLink(LockFreeLinkPolicy::TLinkPtr Item)
{
	GetLockFreeAllocator().Push(Item);
}

LockFreeLinkPolicy::TLinkPtr LockFreeLinkPolicy::AllocLockFreeLink()
{
	LockFreeLinkPolicy::TLinkPtr Result = GetLockFreeAllocator().Pop();
	// this can only really be a mem stomp
	checkLockFreePointerList(Result && !LockFreeLinkPolicy::DerefLink(Result)->DoubleNext.GetPtr() && !LockFreeLinkPolicy::DerefLink(Result)->Payload && !LockFreeLinkPolicy::DerefLink(Result)->SingleNext);
	return Result;
}

LockFreeLinkPolicy::TAllocator LockFreeLinkPolicy::LinkAllocator;

}