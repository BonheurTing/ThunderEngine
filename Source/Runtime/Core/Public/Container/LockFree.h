#pragma once
#pragma optimize("", off)
#include "Assertion.h"
#include "Templates/Alignment.h"
#include "Container.h"
#include "Platform.h"
#include "PlatformProcess.h"
#include "HAL/PlatformAtomics.h"
#include "Memory/MemoryBase.h"
#include "Templates/ThunderTemplates.h"

#define MAX_LOCK_FREE_NODES_AS_BITS (26)
#define MAX_LOCK_FREE_NODES (1 << 26) //supports a maximum of 2^26 nodes
#define checkLockFreePointerList TAssert
#define MAX_TagBitsValue (uint64(1) << (64 - MAX_LOCK_FREE_NODES_AS_BITS))

namespace Thunder
{
	/*
	 * Fill in the structure: Multi-threading reduces cache contention (cache contention)
	 * The usage is to add padding bytes in the structure to prevent cache line conflicts when multiple threads access adjacent data,
	 * thereby improving the performance of multi-threaded programs.
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

	// TLockFreeAllocOnceIndexedAllocator
	template<class T, unsigned int MaxTotalItems, unsigned int ItemsPerPage>
	class TLockFreeAllocOnceIndexedAllocator
	{
		enum
		{
			MaxBlocks = (MaxTotalItems + ItemsPerPage - 1) / ItemsPerPage
		};
	public:

		TLockFreeAllocOnceIndexedAllocator()
		{
			NextIndex.fetch_add(1, std::memory_order_release);
			for (uint32 Index = 0; Index < MaxBlocks; Index++)
			{
				Pages[Index] = nullptr;
			}
		}

		uint32 Alloc(uint32 Count = 1)
		{
			const uint32 FirstItem = NextIndex.fetch_add(Count, std::memory_order_acq_rel);
			if (FirstItem + Count > MaxTotalItems)
			{
				TAssertf(false, "Consumed %d lock free links; there are no more.", MaxTotalItems);
			}
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
			uint32 blockIndex = Index / ItemsPerPage;
			uint32 subIndex = Index % ItemsPerPage;
			TAssert(Index < (uint32)NextIndex.load(std::memory_order_acquire) && Index < MaxTotalItems && blockIndex < MaxBlocks && Pages[blockIndex]);
			return Pages[blockIndex] + subIndex;
		}

	private:
		void* GetRawItem(uint32 Index)
		{
			uint32 blockIndex = Index / ItemsPerPage;
			uint32 subIndex = Index % ItemsPerPage;
			TAssert(Index && Index < (uint32)NextIndex.load(std::memory_order_acquire) && Index < MaxTotalItems && blockIndex < MaxBlocks);
			if (!Pages[blockIndex])
			{
				T* newBlock = new (TMemory::Malloc(ItemsPerPage)) T{};
				TAssert(IsAligned(newBlock, alignof(T)));
				if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Pages[blockIndex], newBlock, nullptr) != nullptr)
				{
					// we lost discard block
					TAssert(Pages[blockIndex] && Pages[blockIndex] != newBlock);
					TMemory::Destroy(newBlock);
				}
				else
				{
					checkLockFreePointerList(Pages[blockIndex]);
				}
			}
			return (void*)(Pages[blockIndex] + subIndex);
		}

		alignas(PLATFORM_CACHE_LINE_SIZE) std::atomic<int32> NextIndex = 0;
		alignas(PLATFORM_CACHE_LINE_SIZE) T* Pages[MaxBlocks];
	};

	/*
	 * FIndexedPointer
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
	 * FIndexedLockFreeLink
	*/
	struct IndexedLockFreeLink 
	{
		AtomicPointer DoubleNext; //是个uint64，是SingleNext的原子版本
		void *Payload; // 实际要装载的数据
		uint32 SingleNext; // 下个节点所在大块内存array的索引
	};

	/*
	 * FLockFreeLinkPolicy
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

	// FLockFreePointerListLIFORoot
	template<int PaddingForCacheContention, uint64 TABAInc = 1>
	class LockFreePointerListLIFORoot : public Noncopyable
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

	// FLockFreePointerListLIFOBase
	template<class T, int PaddingForCacheContention, uint64 TABAInc = 1>
	class LockFreeLIFOListBase : public Noncopyable
	{
		typedef LockFreeLinkPolicy::TDoublePtr TDoublePtr;
		typedef LockFreeLinkPolicy::TLink TLink;
		typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;

	public:
		LockFreeLIFOListBase() = default;

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

	// FLockFreePointerFIFOBase
	template<class T, int PaddingForCacheContention, uint64 TABAInc = 1>
	class LockFreeFIFOListBase : public Noncopyable
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

	template<class T, int PaddingForCacheContention, int NumPriorities>
	class StallingTaskQueue : public Noncopyable
	{
		typedef LockFreeLinkPolicy::TDoublePtr TDoublePtr;
		typedef LockFreeLinkPolicy::TLink TLink;
		typedef LockFreeLinkPolicy::TLinkPtr TLinkPtr;
	public:
		StallingTaskQueue()
		{
			MasterState.Init();
		}
		int32 Push(T* InPayload, uint32 Priority)
		{
			checkLockFreePointerList(Priority < NumPriorities);
			TDoublePtr LocalMasterState;
			LocalMasterState.AtomicRead(MasterState);
			PriorityQueues[Priority].Push(InPayload);
			TDoublePtr NewMasterState;
			NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
			const uint32 Ptr = LocalMasterState.GetPtr();
			int32 ThreadToWake = FindThreadToWake(Ptr);
			if (ThreadToWake >= 0)
			{
				NewMasterState.SetPtr(TurnOffBit(LocalMasterState.GetPtr(), ThreadToWake));
			}
			else
			{
				NewMasterState.SetPtr(LocalMasterState.GetPtr());
			}
			while (!MasterState.InterlockedCompareExchange(NewMasterState, LocalMasterState))
			{
				LocalMasterState.AtomicRead(MasterState);
				NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
				ThreadToWake = FindThreadToWake(LocalMasterState.GetPtr());
				if (ThreadToWake >= 0)
				{
					NewMasterState.SetPtr(TurnOffBit(LocalMasterState.GetPtr(), ThreadToWake));
				}
				else
				{
					NewMasterState.SetPtr(LocalMasterState.GetPtr());
				}
			}
			return ThreadToWake;
		}

		T* Pop(int32 MyThread, bool bAllowStall)
		{
			TAssert(MyThread >= 0 && MyThread < LockFreeLinkPolicy::MAX_BITS_IN_TLinkPtr);

			while (true)
			{
				TDoublePtr LocalMasterState;
				LocalMasterState.AtomicRead(MasterState);
				//checkLockFreePointerList(!TestBit(LocalMasterState.GetPtr(), MyThread) || !FPlatformProcess::SupportsMultithreading()); // you should not be stalled if you are asking for a task
				for (int32 Index = 0; Index < NumPriorities; Index++)
				{
					T *Result = PriorityQueues[Index].Pop();
					if (Result)
					{
						while (true)
						{
							TDoublePtr NewMasterState;
							NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
							NewMasterState.SetPtr(LocalMasterState.GetPtr());
							if (MasterState.InterlockedCompareExchange(NewMasterState, LocalMasterState))
							{
								return Result;
							}
							LocalMasterState.AtomicRead(MasterState);
							checkLockFreePointerList(!TestBit(LocalMasterState.GetPtr(), MyThread) || !FPlatformProcess::SupportsMultithreading()); // you should not be stalled if you are asking for a task
						}
					}
				}
				if (!bAllowStall)
				{
					break; // if we aren't stalling, we are done, the queues are empty
				}
				{
					TDoublePtr NewMasterState;
					NewMasterState.AdvanceCounterAndState(LocalMasterState, 1);
					NewMasterState.SetPtr(TurnOnBit(LocalMasterState.GetPtr(), MyThread));
					if (MasterState.InterlockedCompareExchange(NewMasterState, LocalMasterState))
					{
						break;
					}
				}
			}
			return nullptr;
		}

	private:

		static int32 FindThreadToWake(TLinkPtr Ptr)
		{
			int32 Result = -1;
			UPTRINT Test = UPTRINT(Ptr);
			if (Test)
			{
				Result = 0;
				while (!(Test & 1))
				{
					Test >>= 1;
					Result++;
				}
			}
			return Result;
		}

		static TLinkPtr TurnOffBit(TLinkPtr Ptr, int32 BitToTurnOff)
		{
			return (TLinkPtr)(UPTRINT(Ptr) & ~(UPTRINT(1) << BitToTurnOff));
		}

		static TLinkPtr TurnOnBit(TLinkPtr Ptr, int32 BitToTurnOn)
		{
			return (TLinkPtr)(UPTRINT(Ptr) | (UPTRINT(1) << BitToTurnOn));
		}

		static bool TestBit(TLinkPtr Ptr, int32 BitToTest)
		{
			return !!(UPTRINT(Ptr) & (UPTRINT(1) << BitToTest));
		}

		LockFreeFIFOListBase<T, PaddingForCacheContention> PriorityQueues[NumPriorities];
		// not a pointer to anything, rather tracks the stall state of all threads servicing this queue.
		TDoublePtr MasterState;
		TPaddingForCacheContention<PaddingForCacheContention> PadToAvoidContention1;
	};

	

	// TLockFreePointerListLIFOPad	
	template<class T, int TPaddingForCacheContention>
	class TLockFreeLIFOListNoPad : private LockFreeLIFOListBase<T, TPaddingForCacheContention>
	{
	public:

		void Push(T *NewItem)
		{
			LockFreeLIFOListBase<T, TPaddingForCacheContention>::Push(NewItem);
		}

		T* Pop()
		{
			return LockFreeLIFOListBase<T, TPaddingForCacheContention>::Pop();
		}

		template <typename ContainerType>
		void PopAll(ContainerType& Output)
		{
			LockFreeLIFOListBase<T, TPaddingForCacheContention>::PopAll(Output);
		}

		template <typename FunctorType>
		void PopAllAndApply(FunctorType InFunctor)
		{
			LockFreeLIFOListBase<T, TPaddingForCacheContention>::PopAllAndApply(InFunctor);
		}

		FORCEINLINE bool IsEmpty() const
		{
			return LockFreeLIFOListBase<T, TPaddingForCacheContention>::IsEmpty();
		}
	};

	// TLockFreePointerListLIFO
	template<class T>
	class TLockFreeLIFOList : public TLockFreeLIFOListNoPad<T, 0>
	{

	};

	// TLockFreePointerListUnordered
	template<class T, int TPaddingForCacheContention>
	class TLockFreeListUnordered : public TLockFreeLIFOListNoPad<T, TPaddingForCacheContention>
	{

	};

	
}
