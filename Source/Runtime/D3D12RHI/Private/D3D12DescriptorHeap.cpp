#include "D3D12DescriptorHeap.h"
#include "Assertion.h"
#include "D3D12CommandContext.h"
#include "D3D12RHI.h"

namespace Thunder
{
	D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap, bool bShaderVisible) : TD3D12DeviceChild(device)
	{
		// Initialize heap descriptor.
		Desc = {};
		Desc.Type = type;
		Desc.NumDescriptors = numDescriptorsPerHeap;

		// Determine flags: RTV/DSV are always CPU-only, others depend on bShaderVisible parameter
		if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		{
			Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}
		else
		{
			Desc.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}

		// Create heap.
		const auto hr = ParentDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&RootHeap));
		TAssert(SUCCEEDED(hr));

		// Cache descriptor size and cpu/gpu address.
		DescriptorSize = ParentDevice->GetDescriptorHandleIncrementSize(Desc.Type);
		CPUBase = RootHeap->GetCPUDescriptorHandleForHeapStart();

		// Only shader-visible heaps have GPU handles
		if (bShaderVisible && type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		{
			GPUBase = RootHeap->GetGPUDescriptorHandleForHeapStart();
		}
	}

	D3D12OfflineDescriptorManager::D3D12OfflineDescriptorManager(
		ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32 numDescriptorsPerHeap)
		: TD3D12DeviceChild(device)
		, HeapType(type)
		, NumDescriptorsPerHeap(numDescriptorsPerHeap)
	{
		DescriptorSize = ParentDevice->GetDescriptorHandleIncrementSize(HeapType);
	}

	D3D12OfflineDescriptorManager::~D3D12OfflineDescriptorManager() = default;

	void D3D12OfflineDescriptorManager::AllocateHeap()
	{
		// Create a new non-GPU-visible heap for CPU-side descriptor creation
		auto* newHeap = new D3D12DescriptorHeap(ParentDevice, HeapType, NumDescriptorsPerHeap, false /* CPU-only */);

		const SIZE_T heapBase = newHeap->GetCPUSlotHandle(0).ptr;
		const SIZE_T heapSize = static_cast<SIZE_T>(NumDescriptorsPerHeap) * DescriptorSize;

		const uint32 newHeapIndex = static_cast<uint32>(Heaps.size());

		// Add the new heap with the entire range as free
		Heaps.emplace_back(newHeap, heapBase, heapSize);
		FreeHeaps.push_back(newHeapIndex);
	}

	D3D12OfflineDescriptor D3D12OfflineDescriptorManager::AllocateHeapSlot()
	{
		ScopeLock lock(CriticalSection);

		D3D12OfflineDescriptor result;

		// If no heaps have free space, allocate a new one
		if (FreeHeaps.empty())
		{
			AllocateHeap();
		}

		// Get the first heap with free space
		TAssert(!FreeHeaps.empty());
		const uint32 heapIndex = FreeHeaps.front();
		D3D12OfflineHeapEntry& heapEntry = Heaps[heapIndex];

		// Get the first free range
		TAssert(!heapEntry.FreeList.empty());
		auto& freeRange = heapEntry.FreeList.front();

		// Allocate from the start of this range
		result.Handle.ptr = freeRange.Start;
		result.HeapIndex = heapIndex;

		// Move the start forward
		freeRange.Start += DescriptorSize;

		// If this range is exhausted, remove it
		if (freeRange.Start >= freeRange.End)
		{
			heapEntry.FreeList.pop_front();

			// If this heap has no more free ranges, remove it from FreeHeaps
			if (heapEntry.FreeList.empty())
			{
				FreeHeaps.pop_front();
			}
		}

		return result;
	}

	void D3D12OfflineDescriptorManager::FreeHeapSlot(D3D12OfflineDescriptor& descriptor)
	{
		ScopeLock lock(CriticalSection);

		TAssert(descriptor.HeapIndex < Heaps.size());
		D3D12OfflineHeapEntry& heapEntry = Heaps[descriptor.HeapIndex];

		const SIZE_T freedStart = descriptor.Handle.ptr;
		const SIZE_T freedEnd = descriptor.Handle.ptr + DescriptorSize;

		bool bInserted = false;
		bool bWasEmpty = heapEntry.FreeList.empty();

		// Try to merge with existing ranges (iterate through the list)
		for (auto it = heapEntry.FreeList.begin(); it != heapEntry.FreeList.end(); ++it)
		{
			D3D12OfflineHeapFreeRange& range = *it;

			// Case 1: Freed range is immediately before this range -> extend range backward
			if (range.Start == freedEnd)
			{
				range.Start = freedStart;
				bInserted = true;
				break;
			}
			// Case 2: Freed range is immediately after this range -> extend range forward
			else if (range.End == freedStart)
			{
				range.End = freedEnd;

				// Check if we can merge with the next range
				auto nextIt = std::next(it);
				if (nextIt != heapEntry.FreeList.end() && range.End == nextIt->Start)
				{
					// Merge with next range
					range.End = nextIt->End;
					heapEntry.FreeList.erase(nextIt);
				}

				bInserted = true;
				break;
			}
			// Case 3: Freed range should be inserted before this range
			else if (freedEnd < range.Start)
			{
				heapEntry.FreeList.insert(it, D3D12OfflineHeapFreeRange(freedStart, freedEnd));
				bInserted = true;
				break;
			}
		}

		// Case 4: Not merged with any range, append at the end
		if (!bInserted)
		{
			heapEntry.FreeList.push_back(D3D12OfflineHeapFreeRange(freedStart, freedEnd));
		}

		// If the heap was empty and now has free space, add it back to FreeHeaps
		if (bWasEmpty)
		{
			FreeHeaps.push_back(descriptor.HeapIndex);
		}

		// Clear the descriptor
		descriptor = D3D12OfflineDescriptor();
	}

	D3D12OnlineDescriptorManager::D3D12OnlineDescriptorManager(ID3D12Device* device)
		: TD3D12DeviceChild(device)
	{
	}

	D3D12OnlineDescriptorManager::~D3D12OnlineDescriptorManager()
	{
		// Free all free blocks
		while (!FreeBlocks.empty())
		{
			D3D12OnlineDescriptorBlock* block = FreeBlocks.front();
			FreeBlocks.pop();
			TMemory::Destroy(block);
		}

		// Free all deferred blocks (should not happen if cleanup is correct)
		while (!DeferredDeletionQueue.empty())
		{
			DeferredBlock deferred = DeferredDeletionQueue.front();
			DeferredDeletionQueue.pop();
			TMemory::Destroy(deferred.Block);
		}
	}

	void D3D12OnlineDescriptorManager::Init(uint32 inTotalSize, uint32 inBlockSize)
	{
		TotalSize = inTotalSize;
		BlockSize = inBlockSize;

		// Create the global GPU-visible heap
		GlobalHeap = new D3D12DescriptorHeap(
			ParentDevice,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			TotalSize
		);

		// Split the heap into blocks
		uint32 blockCount = (TotalSize + BlockSize - 1) / BlockSize;
		uint32 currentBaseSlot = 0;

		for (uint32 i = 0; i < blockCount; ++i)
		{
			uint32 actualBlockSize = (i == blockCount - 1) ? (TotalSize - currentBaseSlot) : BlockSize;

			auto* block = new (TMemory::Malloc<D3D12OnlineDescriptorBlock>()) D3D12OnlineDescriptorBlock(currentBaseSlot, actualBlockSize);
			FreeBlocks.push(block);

			currentBaseSlot += actualBlockSize;
		}
	}

	D3D12OnlineDescriptorBlock* D3D12OnlineDescriptorManager::AllocateHeapBlock()
	{
		ScopeLock lock(CriticalSection);

		D3D12OnlineDescriptorBlock* result = nullptr;
		if (!FreeBlocks.empty())
		{
			result = FreeBlocks.front();
			FreeBlocks.pop();
		}

		if (result == nullptr)
		{
			LOG("WARNING: Global descriptor heap exhausted! Switching to local heap.");
		}

		return result;
	}

	void D3D12OnlineDescriptorManager::FreeHeapBlock(D3D12OnlineDescriptorBlock* block)
	{
		TAssert(block != nullptr);

		// Get the current fence value - GPU is still using this block
		auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		uint64 fenceValue = dx12RHI->GetCurrentFenceValue();

		// Add to deferred deletion queue
		ScopeLock lock(CriticalSection);
		DeferredDeletionQueue.push({ block, fenceValue });
	}

	void D3D12OnlineDescriptorManager::ProcessDeferredDeletions()
	{
		auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);

		ScopeLock lock(CriticalSection);

		// Process all completed blocks
		while (!DeferredDeletionQueue.empty())
		{
			DeferredBlock& deferred = DeferredDeletionQueue.front();

			// Check if GPU has finished with this block
			if (dx12RHI->IsFenceComplete(deferred.FenceValue))
			{
				// GPU is done, recycle the block
				Recycle(deferred.Block);
				DeferredDeletionQueue.pop();
			}
			else
			{
				// This block is still in use, stop processing
				// (Queue is in fence order, so all subsequent blocks are also in use)
				break;
			}
		}
	}

	void D3D12OnlineDescriptorManager::Recycle(D3D12OnlineDescriptorBlock* block)
	{
		// Note: CriticalSection should already be locked by caller
		block->SizeUsed = 0;
		FreeBlocks.push(block);
	}

	D3D12OnlineHeap::D3D12OnlineHeap(ID3D12Device* device, bool bInCanLoopAround)
		: TD3D12DeviceChild(device)
		, bCanLoopAround(bInCanLoopAround)
	{
	}

	bool D3D12OnlineHeap::CanReserveSlots(uint32 numSlots) const
	{
		if (Heap == nullptr)
		{
			return false;
		}

		uint32 heapSize = GetTotalSize();

		if (bCanLoopAround)
		{
			// Can loop around, check if we have enough free space
			if (NextSlotIndex >= FirstUsedSlot)
			{
				// [FirstUsed ... NextSlot ... End] or [0 ... End]
				uint32 available = heapSize - NextSlotIndex + FirstUsedSlot;
				return available >= numSlots;
			}
			else
			{
				// [NextSlot ... FirstUsed ... End]
				uint32 available = FirstUsedSlot - NextSlotIndex;
				return available >= numSlots;
			}
		}
		else
		{
			// Cannot loop around, check if we have enough space from current position
			return (NextSlotIndex + numSlots) <= heapSize;
		}
	}

	uint32 D3D12OnlineHeap::ReserveSlots(uint32 numSlotsRequested)
	{
		TAssert(Heap != nullptr);

		uint32 heapSize = GetTotalSize();

		// Check if we need to roll over
		if (!CanReserveSlots(numSlotsRequested))
		{
			bool heapChanged = RollOver();
			if (!heapChanged)
			{
				// If heap didn't change after rollover, we might have looped around
				if (bCanLoopAround && NextSlotIndex != 0)
				{
					HeapLoopedAround();
					NextSlotIndex = 0;
				}
			}
		}

		TAssertf(CanReserveSlots(numSlotsRequested),
			"Failed to reserve %u descriptor slots. Heap size: %u, NextSlot: %u",
			numSlotsRequested, heapSize, NextSlotIndex);

		uint32 ret = NextSlotIndex;
		NextSlotIndex += numSlotsRequested;

		return ret;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12OnlineHeap::GetCPUSlotHandle(uint32 slot) const
	{
		TAssert(Heap != nullptr);
		return Heap->GetCPUSlotHandle(slot);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE D3D12OnlineHeap::GetGPUSlotHandle(uint32 slot) const
	{
		TAssert(Heap != nullptr);
		return Heap->GetGPUSlotHandle(slot);
	}

	D3D12SubAllocatedOnlineHeap::D3D12SubAllocatedOnlineHeap(
		D3D12StateCache* stateCache,
		ID3D12Device* device)
		: D3D12OnlineHeap(device, false) // Cannot loop around
		, StateCache(stateCache)
	{
	}

	D3D12SubAllocatedOnlineHeap::~D3D12SubAllocatedOnlineHeap()
	{
		if (CurrentBlock)
		{
			// Return the block to the manager
			auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
			dx12RHI->GetOnlineDescriptorManager()->FreeHeapBlock(CurrentBlock);
			CurrentBlock = nullptr;
		}
	}

	bool D3D12SubAllocatedOnlineHeap::AllocateBlock()
	{
		auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		auto* onlineManager = dx12RHI->GetOnlineDescriptorManager();

		// Free the old block if we have one
		if (CurrentBlock)
		{
			CurrentBlock->SizeUsed = NextSlotIndex;
			onlineManager->FreeHeapBlock(CurrentBlock);
			CurrentBlock = nullptr;
		}

		// Allocate a new block from the global heap
		CurrentBlock = onlineManager->AllocateHeapBlock();

		// Reset counters
		NextSlotIndex = 0;
		FirstUsedSlot = 0;

		if (CurrentBlock)
		{
			// Use the global heap directly, we'll offset our slot indices
			Heap = onlineManager->GetDescriptorHeap();
			return true;
		}
		else
		{
			// Global heap exhausted, switch to local heap
			Heap.SafeRelease();
			LOG("Global descriptor heap exhausted, switching to local heap.");
			StateCache->SwitchToContextLocalViewHeap();
			return false;
		}
	}

	bool D3D12SubAllocatedOnlineHeap::RollOver()
	{
		return !AllocateBlock();
	}

	void D3D12SubAllocatedOnlineHeap::OpenCommandList()
	{
		if (CurrentBlock == nullptr)
		{
			AllocateBlock();
		}
	}

	void D3D12SubAllocatedOnlineHeap::CloseCommandList()
	{
		// Mark the size used in the current block
		if (CurrentBlock)
		{
			CurrentBlock->SizeUsed = NextSlotIndex;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12SubAllocatedOnlineHeap::GetCPUSlotHandle(uint32 slot) const
	{
		TAssert(Heap != nullptr && CurrentBlock != nullptr);
		// Add the block's base offset to the slot index
		return Heap->GetCPUSlotHandle(CurrentBlock->BaseSlot + slot);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE D3D12SubAllocatedOnlineHeap::GetGPUSlotHandle(uint32 slot) const
	{
		TAssert(Heap != nullptr && CurrentBlock != nullptr);
		// Add the block's base offset to the slot index
		return Heap->GetGPUSlotHandle(CurrentBlock->BaseSlot + slot);
	}

	D3D12LocalOnlineHeap::D3D12LocalOnlineHeap(D3D12StateCache* stateCache, ID3D12Device* device)
		: D3D12OnlineHeap(device, true) // Can loop around
		, StateCache(stateCache)
	{
	}

	D3D12LocalOnlineHeap::~D3D12LocalOnlineHeap() = default;

	void D3D12LocalOnlineHeap::Init(uint32 numDescriptors)
	{
		if (numDescriptors > 0)
		{
			Heap = new D3D12DescriptorHeap(
				ParentDevice,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				numDescriptors
			);

			Entry.Heap = Heap;
			Entry.FenceValue = 0;
		}
	}

	bool D3D12LocalOnlineHeap::RollOver()
	{
		// Add current heap to reclaim pool
		auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
		Entry.FenceValue = dx12RHI->GetCurrentFenceValue();
		ReclaimPool.push(Entry);

		// Try to get a heap from reclaim pool
		bool reuseHeap = false;
		if (!ReclaimPool.empty())
		{
			FPoolEntry tempEntry = ReclaimPool.front();
			if (dx12RHI->IsFenceComplete(tempEntry.FenceValue))
			{
				Entry = ReclaimPool.front();
				ReclaimPool.pop();
				Heap = Entry.Heap;
				reuseHeap = true;
			}
		}

		// If no heap available in pool, create a new one
		if (!reuseHeap)
		{
			LOG("LocalOnlineHeap RollOver: Creating new heap. Consider increasing heap size.");

			uint32 numDescriptors = Heap->GetDesc().NumDescriptors;
			Heap = new D3D12DescriptorHeap(
				ParentDevice,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				numDescriptors
			);

			Entry.Heap = Heap;
		}

		// Reset slot indices
		NextSlotIndex = 0;
		FirstUsedSlot = 0;

		return StateCache->HeapRolledOver();
	}

	void D3D12LocalOnlineHeap::OpenCommandList()
	{
		RecycleSlots();
	}

	void D3D12LocalOnlineHeap::CloseCommandList()
	{
		if (NextSlotIndex > 0)
		{
			// Record sync point for this command list
			FSyncPointEntry syncPoint;
			auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);
			syncPoint.FenceValue = dx12RHI->GetCurrentFenceValue();
			syncPoint.LastSlotInUse = NextSlotIndex - 1;
			SyncPoints.push(syncPoint);

			RecycleSlots();
		}
	}

	void D3D12LocalOnlineHeap::RecycleSlots()
	{
		auto* dx12RHI = static_cast<D3D12DynamicRHI*>(GDynamicRHI);

		// Recycle completed sync points
		while (!SyncPoints.empty())
		{
			FSyncPointEntry syncPoint = SyncPoints.front();
			if (dx12RHI->IsFenceComplete(syncPoint.FenceValue))
			{
				SyncPoints.pop();
				// Update FirstUsedSlot
				FirstUsedSlot = syncPoint.LastSlotInUse + 1;
			}
			else
			{
				break;
			}
		}
	}

	D3D12StateCache::D3D12StateCache(ID3D12Device* device, D3D12CommandContext* context)
		: TD3D12DeviceChild(device)
		, Context(context)
		, SubAllocatedViewHeap(this, device)
	{
		constexpr uint32 kLocalViewHeapSize = 4096; // 4K descriptors.
		NumLocalViewDescriptors = kLocalViewHeapSize;

		// Start with sub-allocated heap
		CurrentViewHeap = &SubAllocatedViewHeap;
	}

	D3D12StateCache::~D3D12StateCache()
	{
		if (LocalViewHeap)
		{
			TMemory::Destroy(LocalViewHeap);
			LocalViewHeap = nullptr;
		}
	}

	void D3D12StateCache::OpenCommandList()
	{
		CurrentViewHeap->OpenCommandList();

		// Always set descriptor heaps at the beginning of command list
		// Command list was reset, so we need to bind heaps again
		SetDescriptorHeaps();
	}

	void D3D12StateCache::CloseCommandList()
	{
		CurrentViewHeap->CloseCommandList();
	}

	bool D3D12StateCache::SwitchToContextLocalViewHeap()
	{
		// Lazy initialize local view heap
		if (LocalViewHeap == nullptr)
		{
			LOG("Allocating local view heap - global heap exhausted!");
			LocalViewHeap = new (TMemory::Malloc<D3D12LocalOnlineHeap>()) D3D12LocalOnlineHeap(this, ParentDevice);
			LocalViewHeap->Init(NumLocalViewDescriptors);
		}

		// Close old heap, open new heap
		CurrentViewHeap->CloseCommandList();
		CurrentViewHeap = LocalViewHeap;
		CurrentViewHeap->OpenCommandList();

		// Update descriptor heaps on command list
		bool heapChanged = SetDescriptorHeaps();
		TAssert(LastSetViewHeap == CurrentViewHeap->GetHeap()->GetHeap());

		return heapChanged;
	}

	bool D3D12StateCache::HeapRolledOver()
	{
		return SetDescriptorHeaps();
	}

	bool D3D12StateCache::SetDescriptorHeaps()
	{
		auto* commandList = Context->GetCommandList().Get();
		auto* currentHeap = CurrentViewHeap->GetHeap()->GetHeap();

		bool heapChanged = (LastSetViewHeap != currentHeap);

		if (heapChanged)
		{
			ID3D12DescriptorHeap* heaps[] = { currentHeap };
			commandList->SetDescriptorHeaps(1, heaps);
			LastSetViewHeap = currentHeap;
		}

		return heapChanged;
	}

	void D3D12StateCache::Reset()
	{
		// Clear LastSetViewHeap to force re-binding.
		LastSetViewHeap = nullptr;

		OpenCommandList();

		// Reset root signature.
		LastShaderRC = {};
		CachedRootSignature = nullptr;
	}
}

