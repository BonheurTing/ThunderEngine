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
	//todo MS_ALIGN(8)
	struct AtomicPointer
	{
		void Init()
		{
			Ptrs[0] = 0;
			Ptrs[1] = 0;
		}

		FORCEINLINE void SetAllField(uint64 ptr, uint64 counterAndState)
		{
			Ptrs[0] = ptr;
			Ptrs[1] = counterAndState;
		}

		FORCEINLINE uint64 GetPtr() const
		{
			return Ptrs[0];
		}

		FORCEINLINE void SetPtr(uint64 in)
		{
			SetAllField(in, GetCounterAndState());
		}

		FORCEINLINE uint64 GetCounterAndState() const
		{
			return Ptrs[1];
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

		FORCEINLINE void AtomicRead(AtomicPointer& other) //todo
		{
			checkLockFreePointerList(IsAligned(&Ptrs, 8) && IsAligned(&other.Ptrs, 8));
			::InterlockedCompareExchange128((volatile int64*)other.Ptrs, Ptrs[0], Ptrs[1], 0);
			//todo TestCriticalStall();
		}

		FORCEINLINE bool InterlockedCompareExchange(const AtomicPointer& Exchange, const AtomicPointer& Comparand)
		{
			//todo TestCriticalStall();
			return ::InterlockedCompareExchange128((volatile int64*)Ptrs, Exchange.Ptrs[0], Exchange.Ptrs[1], (int64*)Comparand.Ptrs);
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
		uint64 Ptrs[2] {0};
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
