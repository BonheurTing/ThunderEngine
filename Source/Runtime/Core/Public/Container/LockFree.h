#pragma once
#include "Assertion.h"
#include "Templates/Alignment.h"
#include "Container.h"
#include "Platform.h"
#include "PlatformProcess.h"
#include "HAL/PlatformAtomics.h"
#include "Memory/MemoryBase.h"

#define MAX_LOCK_FREE_NODES_AS_BITS (26)
#define MAX_LOCK_FREE_NODES (1 << 26) //最多支持2^26个节点
#define checkLockFreePointerList TAssert
#define MAX_TagBitsValue (uint64(1) << (64 - MAX_LOCK_FREE_NODES_AS_BITS))

namespace Thunder
{
	
/*
 * 填充结构体:多线程重减少缓存争用(cache contention)
 * 用法是结构体中添加填充字节，以避免多个线程在访问相邻的数据时产生缓存行（cache line）的竞争，从而提高多线程程序的性能
 **/
template<int PaddingForCacheContention>
struct TPaddingForCacheContention
{
	uint8 PadToAvoidContention[PaddingForCacheContention];
};

template<>
struct TPaddingForCacheContention<0>
{
};

template<class T, unsigned int MaxTotalItems, unsigned int ItemsPerPage>
class TLockFreeAllocOnceIndexedAllocator
{
	//unsigned int MaxTotalItems;
	//unsigned int ItemsPerPage;
	
	enum
	{
		MaxBlocks = 10 //(MaxTotalItems + ItemsPerPage - 1) / ItemsPerPage
	};
public:

	TLockFreeAllocOnceIndexedAllocator()
	{
		NextIndex.fetch_add(1, std::memory_order_relaxed);
		for (uint32 Index = 0; Index < MaxBlocks; Index++)
		{
			Pages[Index] = nullptr;
		}
	}

	FORCEINLINE uint32 Alloc(uint32 Count = 1)
	{
		const uint32 FirstItem = NextIndex.fetch_add(Count, std::memory_order_relaxed);
		TAssertf(FirstItem + Count > MaxTotalItems, "Consumed %d lock free links; there are no more.", MaxTotalItems);
		for (uint32 CurrentItem = FirstItem; CurrentItem < FirstItem + Count; CurrentItem++)
		{
			new (GetRawItem(CurrentItem)) T();
		}
		return FirstItem;
	}
	FORCEINLINE T* GetItem(uint32 Index)
	{
		if (!Index)
		{
			return nullptr;
		}
		uint32 BlockIndex = Index / ItemsPerPage;
		uint32 SubIndex = Index % ItemsPerPage;
		TAssert(Index < (uint32)NextIndex.load() && Index < MaxTotalItems && BlockIndex < MaxBlocks && Pages[BlockIndex]);
		return Pages[BlockIndex] + SubIndex;
	}

private:
	void* GetRawItem(uint32 Index)
	{
		uint32 BlockIndex = Index / ItemsPerPage;
		uint32 SubIndex = Index % ItemsPerPage;
		TAssert(Index && Index < (uint32)NextIndex.load() && Index < MaxTotalItems && BlockIndex < MaxBlocks);
		if (!Pages[BlockIndex])
		{
			T* NewBlock = new (TMemory::Malloc<T>(ItemsPerPage)) T{};
			TAssert(IsAligned(NewBlock, alignof(T)));
			if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Pages[BlockIndex], NewBlock, nullptr) != nullptr)
			{
				// we lost discard block
				TAssert(Pages[BlockIndex] && Pages[BlockIndex] != NewBlock);
				TMemory::Destroy(NewBlock);
			}
			else
			{
				checkLockFreePointerList(Pages[BlockIndex]);
			}
		}
		return (void*)(Pages[BlockIndex] + SubIndex);
	}

	alignas(PLATFORM_CACHE_LINE_SIZE) std::atomic<int32> NextIndex = 0;
	alignas(PLATFORM_CACHE_LINE_SIZE) T* Pages[MaxBlocks];
};

/*
 * 支持原子操作的节点指针（64位，低26位表示指针索引，其他高位表示计数器或状态），最多支持2^26个节点
 * no constructor, intentionally. We need to keep the ABA double counter in tact
 * This should only be used for FIndexedPointer's with no outstanding concurrency.
 * Not recycled links, for example.
*/
//todo MS_ALIGN(8)
struct AtomicPointer
{
	void Init()
	{
		TAssertf(((MAX_LOCK_FREE_NODES - 1) & MAX_LOCK_FREE_NODES) == 0, "MAX_LOCK_FREE_LINKS must be a power of two");
		Ptrs = 0;
	}

	FORCEINLINE void SetAllField(uint32 ptr, uint64 counterAndState)
	{
		checkLockFreePointerList(ptr < MAX_LOCK_FREE_NODES && counterAndState < (uint64(1) << (64 - MAX_LOCK_FREE_NODES_AS_BITS)));
		Ptrs = (uint64(ptr) | (counterAndState << MAX_LOCK_FREE_NODES_AS_BITS)); // 64位，低26位表示指针索引，其他高位表示计数器或状态
	}

	FORCEINLINE uint32 GetPtr() const
	{
		return uint32(Ptrs & (MAX_LOCK_FREE_NODES - 1)); //低26位
	}

	FORCEINLINE void SetPtr(uint32 in)
	{
		SetAllField(in, GetCounterAndState());
	}

	FORCEINLINE uint64 GetCounterAndState() const
	{
		return (Ptrs >> MAX_LOCK_FREE_NODES_AS_BITS); //高38位
	}

	FORCEINLINE void SetCounterAndState(uint64 in)
	{
		SetAllField(GetPtr(), in);
	}

	FORCEINLINE void AdvanceCounterAndState(const AtomicPointer &From, uint64 TABAInc)
	{
		SetCounterAndState(From.GetCounterAndState() + TABAInc);
		if (!!(GetCounterAndState() < From.GetCounterAndState()))
		{
			// this is not expected to be a problem and it is not expected to happen very often. When it does happen, we will sleep as an extra precaution.
			FPlatformProcess::Sleep(.001f);
		}
	}

	/*
	* TABAInc = 1 << (26 + ABA_Count_BitS)
	* return: ABA Data (27 ~ 26+ABA_Count_BitS)
	*/
	template<uint64 TABAInc>
	FORCEINLINE uint64 GetState() const
	{
		return GetCounterAndState() & (TABAInc - 1);
	}

	/*
	* (GetCounterAndState() & ~(TABAInc - 1)): 获取干净的 高于26+ABA_Count_BitS后的bit数据
	* | value: 设置ABA Data (27 ~ 26+ABA_Count_BitS)
	*/
	template<uint64 TABAInc>
	FORCEINLINE void SetState(uint64 value)
	{
		checkLockFreePointerList(value < TABAInc);
		SetCounterAndState((GetCounterAndState() & ~(TABAInc - 1)) | value);
	}

	/*
	* 把other的数据读到自己的Ptrs中
	* AtomicRead主要是用的 InterlockedCompareExchange(ptr, 0, 0)的返回值，这个函数是ptr指向的值和参数三作比较，相同则赋值参数二，
	* 另外无论如何都会返回ptr指向的原始值，所以需要的是这个返回值，随便写个交换值和比较值相同的参数即可
	*/
	FORCEINLINE void AtomicRead(const AtomicPointer& other)
	{
		checkLockFreePointerList(IsAligned(&Ptrs, 8) && IsAligned(&other.Ptrs, 8));
		Ptrs = uint64(FPlatformAtomics::AtomicRead((volatile const int64*)&other.Ptrs));
		//todo TestCriticalStall();
	}

	FORCEINLINE bool InterlockedCompareExchange(const AtomicPointer& Exchange, const AtomicPointer& Comparand)
	{
		//todo TestCriticalStall();
		return uint64(FPlatformAtomics::InterlockedCompareExchange((volatile int64*)&Ptrs, Exchange.Ptrs, Comparand.Ptrs)) == Comparand.Ptrs;
	}
	
	FORCEINLINE bool operator==(const AtomicPointer& Other) const
	{
		return Ptrs == Other.Ptrs;
	}
	FORCEINLINE bool operator!=(const AtomicPointer& Other) const
	{
		return Ptrs != Other.Ptrs;
	}

private:
	uint64 Ptrs;
}; //todo GCC_ALIGN(8)

/*
 *
 * 
*/
struct IndexedLockFreeLink 
{
	AtomicPointer DoubleNext; //是个uint64，是SingleNext的原子版本
	void *Payload; // 实际要装载的数据
	uint32 SingleNext; // 下个节点所在大块内存array的索引
};

/*
 * 
 * 
*/
struct LockFreeLinkPolicy
{
	enum
	{
		MAX_BITS_IN_TLinkPtr = MAX_LOCK_FREE_NODES_AS_BITS
	};
	typedef AtomicPointer TDoublePtr; // 可以原子操作的指针，地26位地址是地址(其实就是分配的大块内存的索引TLinkPtr), 高位计数 // 作为head
	typedef IndexedLockFreeLink TLink; // 队列的单个节点
	typedef uint32 TLinkPtr; //分配的一大块内存array的索引
	typedef TLockFreeAllocOnceIndexedAllocator<IndexedLockFreeLink, MAX_LOCK_FREE_NODES, 16384> TAllocator;
	// 无锁队列的操作只对head，数据，next，head和next都保证原子性才能多线程安全，这俩都是指向node的指针，因此实现一个AtomicPointer来保证原子性
	// IndexedLockFreeLink节点，todo: single next的用处

	static FORCEINLINE IndexedLockFreeLink* DerefLink(uint32 Ptr) 
	{
		return LinkAllocator.GetItem(Ptr);
	}
	// 
	static FORCEINLINE IndexedLockFreeLink* IndexToLink(uint32 Index)
	{
		return LinkAllocator.GetItem(Index);
	}
	// 
	static FORCEINLINE uint32 IndexToPtr(uint32 Index)
	{
		return Index;
	}

	/*
	 * 向队列申请分配一个Node来存储数据
	 * 初始的时候申请一大块，来一个节点就分配一直地址，而不是每次新node都直接TMemory::Malloc
	 * 因为现在的内存管理器虽然支持多线程，但是无锁队列要解决ABA问题需要计数，需要低26位是地址，高位是计数状态，内存管理器不能保证高位都是0
	 * 因此先申请一大块再做映射
	*/
	CORE_API static uint32 AllocLockFreeLink();
	CORE_API static void FreeLockFreeLink(uint32 Item);
	CORE_API static TAllocator LinkAllocator;
};

template<int PaddingForCacheContention, uint64 TABAInc = 1>
class LockFreePointerListLIFORoot
{
	typedef LockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef LockFreeLinkPolicy::TLink TLink;
	typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;

public:
	FORCEINLINE LockFreePointerListLIFORoot()
	{
		// We want to make sure we have quite a lot of extra counter values to avoid the ABA problem. This could probably be relaxed, but eventually it will be dangerous. 
		// The question is "how many queue operations can a thread starve for".
		TAssertf(MAX_TagBitsValue / TABAInc >= (1 << 23), "risk of ABA problem");
		TAssertf((TABAInc & (TABAInc - 1)) == 0, "must be power of two");
		Reset();
	}

	void Reset()
	{
		Head.Init();
	}

	void Push(TLinkPtr Item)
	{
		while (true)
		{
			// 从内存中拿出Head指针
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);

			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc); //ABA计数+1
			NewHead.SetPtr(Item); //设置节点
			LockFreeLinkPolicy::DerefLink(Item)->SingleNext = LocalHead.GetPtr(); //设置next为原来的head指向的节点地址
			if (Head.InterlockedCompareExchange(NewHead, LocalHead)) //Head和LocalHead比较，如果一致就赋值成NewHead；返回Head指向的内容和LocalHead做比较，如果相同就说明刚才交换成功了，break
			{
				break;
			}
		}
	}

	TLinkPtr Pop()
	{
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			if (!Item)
			{
				break;
			}
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			TLink* ItemP = LockFreeLinkPolicy::DerefLink(Item);
			NewHead.SetPtr(ItemP->SingleNext);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				ItemP->SingleNext = 0;
				break;
			}
		}
		return Item;
	}

	TLinkPtr PopAll()
	{
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			if (!Item)
			{
				break;
			}
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
			NewHead.SetPtr(0);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				break;
			}
		}
		return Item;
	}

	FORCEINLINE bool IsEmpty() const
	{
		return !Head.GetPtr();
	}

	FORCEINLINE uint64 GetState() const
	{
		TDoublePtr LocalHead;
		LocalHead.AtomicRead(Head);
		return LocalHead.GetState<TABAInc>();
	}

private:

	TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention1;
	TDoublePtr Head;
	TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention2;
};

template<class T, int PaddingForCacheContention, uint64 TABAInc = 1>
class LockFreeLIFOListBase
{
	typedef LockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef LockFreeLinkPolicy::TLink TLink;
	typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;

public:
	LockFreeLIFOListBase()
	{
		// We want to make sure we have quite a lot of extra counter values to avoid the ABA problem. This could probably be relaxed, but eventually it will be dangerous. 
		// The question is "how many queue operations can a thread starve for".
		TAssertf(MAX_TagBitsValue / TABAInc >= (1 << 23), "risk of ABA problem");
		TAssertf((TABAInc & (TABAInc - 1)) == 0, "must be power of two");
		RootList.Init();
	}

	~LockFreeLIFOListBase()
	{
		while (Pop()) {};
	}

	void Reset()
	{
		while (Pop()) {};
		RootList.Init();
	}

	void Push(T* InPayload)
	{
		//向队列申请分配一个Node来存储数据
		TLinkPtr Item = LockFreeLinkPolicy::AllocLockFreeLink();
		// TLinkPtr是映射后的索引，所以对node操作都要先解引用
		LockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
		RootList.Push(Item);
	}

	T* Pop()
	{
		TLinkPtr Item = RootList.Pop();
		T* Result = nullptr;
		if (Item)
		{
			Result = (T*)LockFreeLinkPolicy::DerefLink(Item)->Payload;
			LockFreeLinkPolicy::FreeLockFreeLink(Item);
		}
		return Result;
	}
	
	/*void Push(T* InPayload)
	{
		//向队列申请分配一个Node来存储数据
		TLinkPtr Item = LockFreeLinkPolicy::AllocLockFreeLink();
		// TLinkPtr是映射后的索引，所以对node操作都要先解引用
		LockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
		
		while (true)
		{
			// 从内存中拿出Head指针
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc); //ABA计数+1
			NewHead.SetPtr(Item); //设置节点
			LockFreeLinkPolicy::DerefLink(Item)->SingleNext = LocalHead.GetPtr(); //设置next为原来的head指向的节点地址
			
			if (Head.InterlockedCompareExchange(NewHead, LocalHead)) //Head和LocalHead比较，如果一致就赋值成NewHead；返回Head指向的内容和LocalHead做比较，如果相同就说明刚才交换成功了，break
			{
				break;
			}
			
			/*
			提问：每次新node都正常申请64位，64位计数，每次都原子操作128位可以吗
			// 从内存中拿出Head指针
			TDoublePtr localHead;
			localHead.AtomicRead(Head);
			
			TNode* newNode = LockFreeLinkPolicy_Old<T>::AllocLockFreeLink(item);
			
			TDoublePtr newHead;
			// 更新用于防止ABA问题的计数器
			newHead.AdvanceCounterAndState(localHead, TABAInc);
			// 更新NewHead指向LocalHead
			newHead.SetPtr(&newNode);
			newNode->Next = localHead.GetPtr();
			// CAS原子判定（指针和计数器，合在一起是uint64，直接用CAS比较）
			// 这样其他线程中的Push/Pop操作没法重入，Pop中的实现类似
			::InterlockedCompareExchange128((volatile int64*)&Head, newHead.GetCounterAndState(), newHead.GetPtr(), (int64*)&localHead);
		
			if (Head.InterlockedCompareExchange(newHead, localHead))
			{
				break;
			}
			#1#
		}
	}

	T* Pop()
	{
		TLinkPtr Item = 0;
		while (true)
		{
			TDoublePtr LocalHead;
			LocalHead.AtomicRead(Head);
			Item = LocalHead.GetPtr();
			if (!Item)
			{
				break;
			}
			TDoublePtr NewHead;
			NewHead.AdvanceCounterAndState(LocalHead, TABAInc); //todo: 这个计数是怎么保证原子的，咋push和pop都是+1
			TLink* ItemP = LockFreeLinkPolicy::DerefLink(Item);
			NewHead.SetPtr(ItemP->SingleNext);
			if (Head.InterlockedCompareExchange(NewHead, LocalHead))
			{
				ItemP->SingleNext = 0;
				break;
			}
		}

		T* Result = nullptr;
		if (Item) // >0 todo: check 0 is clipped
		{
			Result = (T*)LockFreeLinkPolicy::DerefLink(Item)->Payload;
			LockFreeLinkPolicy::FreeLockFreeLink(Item);
		}
		return Result;
	}*/

	bool IsEmpty() const
	{
		return !RootList.GetPtr();
	}

	uint64 GetState() const
	{
		return RootList.GetState();
	}
	
private:
	LockFreePointerListLIFORoot<PaddingForCacheContention, TABAInc> RootList;
};

template<class T, int PaddingForCacheContention, uint64 TABAInc = 1>
class LockFreeFIFOListBase
{
	typedef LockFreeLinkPolicy::TDoublePtr TDoublePtr;
	typedef LockFreeLinkPolicy::TLink TLink;
	typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;
public:

	FORCEINLINE LockFreeFIFOListBase()
	{
		// We want to make sure we have quite a lot of extra counter values to avoid the ABA problem. This could probably be relaxed, but eventually it will be dangerous. 
		// The question is "how many queue operations can a thread starve for".
		static_assert(TABAInc <= 65536, "risk of ABA problem");
		static_assert((TABAInc & (TABAInc - 1)) == 0, "must be power of two");

		Head.Init();
		Tail.Init();
		TLinkPtr Stub = LockFreeLinkPolicy::AllocLockFreeLink();
		Head.SetPtr(Stub);
		Tail.SetPtr(Stub);
	}

	~LockFreeFIFOListBase()
	{
		while (Pop()) {};
		LockFreeLinkPolicy::FreeLockFreeLink(Head.GetPtr());
	}

	void Push(T* InPayload)
	{
		TLinkPtr Item = LockFreeLinkPolicy::AllocLockFreeLink();
		LockFreeLinkPolicy::DerefLink(Item)->Payload = InPayload;
		TDoublePtr LocalTail;
		while (true)
		{
			LocalTail.AtomicRead(Tail);
			TLink* LocalTailP = LockFreeLinkPolicy::DerefLink(LocalTail.GetPtr());
			TDoublePtr LocalNext;
			LocalNext.AtomicRead(LocalTailP->DoubleNext);
			TDoublePtr TestLocalTail;
			TestLocalTail.AtomicRead(Tail);
			if (TestLocalTail == LocalTail)
			{
				if (LocalNext.GetPtr())
				{
					//TestCriticalStall();
					TDoublePtr NewTail;
					NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
					NewTail.SetPtr(LocalNext.GetPtr());
					Tail.InterlockedCompareExchange(NewTail, LocalTail);
				}
				else
				{
					//TestCriticalStall();
					TDoublePtr NewNext;
					NewNext.AdvanceCounterAndState(LocalNext, TABAInc);
					NewNext.SetPtr(Item);
					if (LocalTailP->DoubleNext.InterlockedCompareExchange(NewNext, LocalNext))
					{
						break;
					}
				}
			}
		}
		{
			//TestCriticalStall();
			TDoublePtr NewTail;
			NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
			NewTail.SetPtr(Item);
			Tail.InterlockedCompareExchange(NewTail, LocalTail);
		}
	}

	T* Pop()
	{
		T* Result = nullptr;
		TDoublePtr LocalHead;
		while (true)
		{
			LocalHead.AtomicRead(Head);
			TDoublePtr LocalTail;
			LocalTail.AtomicRead(Tail);
			TDoublePtr LocalNext;
			LocalNext.AtomicRead(LockFreeLinkPolicy::DerefLink(LocalHead.GetPtr())->DoubleNext);
			TDoublePtr LocalHeadTest;
			LocalHeadTest.AtomicRead(Head);
			if (LocalHead == LocalHeadTest)
			{
				if (LocalHead.GetPtr() == LocalTail.GetPtr())
				{
					if (!LocalNext.GetPtr())
					{
						return nullptr;
					}
					//TestCriticalStall();
					TDoublePtr NewTail;
					NewTail.AdvanceCounterAndState(LocalTail, TABAInc);
					NewTail.SetPtr(LocalNext.GetPtr());
					Tail.InterlockedCompareExchange(NewTail, LocalTail);
				}
				else
				{
					//TestCriticalStall();
					Result = (T*)LockFreeLinkPolicy::DerefLink(LocalNext.GetPtr())->Payload;
					TDoublePtr NewHead;
					NewHead.AdvanceCounterAndState(LocalHead, TABAInc);
					NewHead.SetPtr(LocalNext.GetPtr());
					if (Head.InterlockedCompareExchange(NewHead, LocalHead))
					{
						break;
					}
				}
			}
		}
		LockFreeLinkPolicy::FreeLockFreeLink(LocalHead.GetPtr());
		return Result;
	}

	void PopAll(TArray<T*>& OutArray)
	{
		while (T* Item = Pop())
		{
			OutArray.Add(Item);
		}
	}


	FORCEINLINE bool IsEmpty() const
	{
		TDoublePtr LocalHead;
		LocalHead.AtomicRead(Head);
		TDoublePtr LocalNext;
		LocalNext.AtomicRead(LockFreeLinkPolicy::DerefLink(LocalHead.GetPtr())->DoubleNext);
		return !LocalNext.GetPtr();
	}

private:

	TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention1;
	TDoublePtr Head;
	TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention2;
	TDoublePtr Tail;
	TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention3;
};
}
