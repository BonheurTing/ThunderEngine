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

namespace Thunder
{
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
			static_assert(((MAX_LOCK_FREE_NODES - 1) & MAX_LOCK_FREE_NODES) == 0, "MAX_LOCK_FREE_LINKS must be a power of two");
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

	template<typename T>
	struct LockFreeLink
	{
		LockFreeLink* Next;
		T* Value;
	};

	template<typename T>
	struct LockFreeLinkPolicy
	{
		typedef AtomicPointer TDoublePtr;
		typedef LockFreeLink<T> TNode;
		
		CORE_API static TNode* AllocLockFreeLink(T* nodeValue)
		{
			return new (TMemory::Malloc<TNode>()) TNode{ nullptr, nodeValue };
		}
		CORE_API static void FreeLockFreeLink(TNode* node)
		{
			TMemory::Destroy(node->Value);
			TMemory::Destroy(node);
		}
	};

	template<typename T, uint64 TABAInc = 1>
	class LockFreeLIFOListBase
	{ 
		typedef typename LockFreeLinkPolicy<T>::TDoublePtr TDoublePtr;
		typedef LockFreeLink<T> TNode;

	public:
		FORCEINLINE LockFreeLIFOListBase()
		{
			Head.Init();
		}
		
		void Push(T* item)
		{
			while (true)
			{
				// 从内存中拿出Head指针
				TDoublePtr localHead;
				localHead.AtomicRead(Head);
				TNode* newNode = LockFreeLinkPolicy<T>::AllocLockFreeLink(item);
				
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
			}
		}

		T* Pop()
		{
			uint64 Item = 0;
			while (true)
			{
				TDoublePtr localHead;
				localHead.AtomicRead(Head);
				Item = localHead.GetPtr();
				if (!Item)
				{
					break;
				}
				TDoublePtr NewHead;
				NewHead.AdvanceCounterAndState(localHead, TABAInc);
				TNode* ItemP = (TNode*)Item;
				NewHead.SetPtr(ItemP->Next);
				if (Head.InterlockedCompareExchange(NewHead, localHead))
				{
					ItemP->SingleNext = 0;
					break;
				}
			}
			return Item;
		}
	private:
		TDoublePtr Head;
	};

	class LockFreeFIFOListBase
	{
	};
}
