#pragma once
#include "CoreMinimal.h"
#include "D3D12RHICommon.h"
#include "d3dx12.h"
#include "RHI.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
	class D3D12DescriptorHeap : public TD3D12DeviceChild
	{
	public:
		D3D12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap);

		FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 slot) const { return{ CPUBase.ptr + static_cast<SIZE_T>(slot) * DescriptorSize }; }
		FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 slot) const { return{ GPUBase.ptr + static_cast<SIZE_T>(slot) * DescriptorSize }; }

		FORCEINLINE uint32 GetDescriptorSize() const { return DescriptorSize; }
		FORCEINLINE ID3D12DescriptorHeap* GetHeap() const { return RootHeap.Get(); }
		FORCEINLINE const D3D12_DESCRIPTOR_HEAP_DESC& GetDesc() const { return Desc; }

	private:
		uint32 DescriptorSize;
		D3D12_DESCRIPTOR_HEAP_DESC Desc;
		ComPtr<ID3D12DescriptorHeap> RootHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE CPUBase{};
		D3D12_GPU_DESCRIPTOR_HANDLE GPUBase{};
	};

	// Extended CPU descriptor handle with heap tracking
	struct D3D12OfflineDescriptor
	{
		D3D12_CPU_DESCRIPTOR_HANDLE Handle{ 0 };
		uint32 HeapIndex = 0;  // Which heap this descriptor belongs to

		D3D12OfflineDescriptor() = default;
		D3D12OfflineDescriptor(SIZE_T inPtr, uint32 inHeapIndex) : Handle{ inPtr }, HeapIndex(inHeapIndex) {}
	};

	// Free range in a descriptor heap (range-based allocation for efficiency)
	struct D3D12OfflineHeapFreeRange
	{
		SIZE_T Start;  // Byte address (not index)
		SIZE_T End;    // Byte address (not index)

		D3D12OfflineHeapFreeRange(SIZE_T inStart, SIZE_T inEnd) : Start(inStart), End(inEnd) {}
	};

	// Single offline heap entry with free range tracking
	struct D3D12OfflineHeapEntry
	{
		TRefCountPtr<D3D12DescriptorHeap> Heap;
		TList<D3D12OfflineHeapFreeRange> FreeList;  // List of free ranges (NOT individual slots!)

		D3D12OfflineHeapEntry() = default;
		D3D12OfflineHeapEntry(D3D12DescriptorHeap* inHeap, SIZE_T heapBase, SIZE_T heapSize)
			: Heap(inHeap)
		{
			// Initially, the entire heap is one big free range
			FreeList.emplace_back(heapBase, heapBase + heapSize);
		}
	};

	// Manages CPU-side descriptor allocation (persistent, used during resource creation)
	class D3D12OfflineDescriptorManager : public TD3D12DeviceChild
	{
	public:
		D3D12OfflineDescriptorManager(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap);
		~D3D12OfflineDescriptorManager();

		// Allocate a CPU descriptor handle
		D3D12OfflineDescriptor AllocateHeapSlot();

		// Free a CPU descriptor handle (supports automatic range merging)
		void FreeHeapSlot(D3D12OfflineDescriptor& descriptor);

	private:
		void AllocateHeap();

		D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
		uint32 NumDescriptorsPerHeap;
		uint32 DescriptorSize;

		TArray<D3D12OfflineHeapEntry> Heaps;
		TList<uint32> FreeHeaps; // Indices of heaps with free space

		Mutex CriticalSection;
	};

	// RHI descriptors.
	class D3D12RHIShaderResourceView : public RHIShaderResourceView
	{
	public:
		D3D12RHIShaderResourceView(RHIViewDescriptor const& desc, D3D12OfflineDescriptor const& offlineDescriptor)
			: RHIShaderResourceView(desc, offlineDescriptor.Handle.ptr, offlineDescriptor.HeapIndex) {}
	};

	class D3D12RHIUnorderedAccessView : public RHIUnorderedAccessView
	{
	public:
		D3D12RHIUnorderedAccessView(RHIViewDescriptor const& desc, D3D12OfflineDescriptor const& offlineDescriptor)
			: RHIUnorderedAccessView(desc, offlineDescriptor.Handle.ptr, offlineDescriptor.HeapIndex) {}
	};

	class D3D12RHIRenderTargetView : public RHIRenderTargetView
	{
	public:
		D3D12RHIRenderTargetView(RHIViewDescriptor const& desc, D3D12OfflineDescriptor const& offlineDescriptor)
			: RHIRenderTargetView(desc, offlineDescriptor.Handle.ptr, offlineDescriptor.HeapIndex) {}
	};

	class D3D12RHIDepthStencilView : public RHIDepthStencilView
	{
	public:
		D3D12RHIDepthStencilView(RHIViewDescriptor const& desc, D3D12OfflineDescriptor const& offlineDescriptor)
			: RHIDepthStencilView(desc, offlineDescriptor.Handle.ptr, offlineDescriptor.HeapIndex) {}
	};

	class D3D12RHIConstantBufferView : public RHIConstantBufferView
	{
	public:
		D3D12RHIConstantBufferView(D3D12OfflineDescriptor const& offlineDescriptor)
			: RHIConstantBufferView({}, offlineDescriptor.Handle.ptr, offlineDescriptor.HeapIndex) {}
	};

	class D3D12RHISampler : public RHISampler
	{
	public:
		D3D12RHISampler(RHISamplerDescriptor const& desc, D3D12OfflineDescriptor const& offlineDescriptor)
			: RHISampler(desc), OfflineDescriptor(offlineDescriptor) {}

	private:
		D3D12OfflineDescriptor OfflineDescriptor;
	};

	// Online heaps.
	class D3D12CommandContext;
	class D3D12DescriptorCache;
	class D3D12OnlineDescriptorManager;

	// Represents a sub-allocated block from the global descriptor heap.
	struct D3D12OnlineDescriptorBlock
	{
		uint32 BaseSlot = 0;      // Starting slot in the global heap
		uint32 Size = 0;          // Size of this block
		uint32 SizeUsed = 0;      // Actually used size in this block

		D3D12OnlineDescriptorBlock() = default;
		D3D12OnlineDescriptorBlock(uint32 inBaseSlot, uint32 inSize)
			: BaseSlot(inBaseSlot), Size(inSize) {}
	};

	// Manages the global descriptor heap and allocates blocks to contexts.
	class D3D12OnlineDescriptorManager : public TD3D12DeviceChild
	{
	public:
		D3D12OnlineDescriptorManager(ID3D12Device* device);
		~D3D12OnlineDescriptorManager();

		// Initialize the global heap with total size and block size
		void Init(uint32 inTotalSize, uint32 inBlockSize);

		// Allocate a block from the global heap
		D3D12OnlineDescriptorBlock* AllocateHeapBlock();

		// Free a block back to the free list (delayed recycle)
		void FreeHeapBlock(D3D12OnlineDescriptorBlock* block);

		// Process deferred deletions - call this every frame
		void ProcessDeferredDeletions();

		// Get the global heap
		D3D12DescriptorHeap* GetDescriptorHeap() const { return GlobalHeap.Get(); }

	private:
		// Actually recycle the block (called after GPU finishes)
		void Recycle(D3D12OnlineDescriptorBlock* block);

		// Deferred deletion entry
		struct DeferredBlock
		{
			D3D12OnlineDescriptorBlock* Block;
			uint64 FenceValue;
		};

		TRefCountPtr<D3D12DescriptorHeap> GlobalHeap;
		TQueue<D3D12OnlineDescriptorBlock*> FreeBlocks;
		TQueue<DeferredBlock> DeferredDeletionQueue;
		Mutex CriticalSection;

		uint32 TotalSize = 0;
		uint32 BlockSize = 0;
	};

	// Base class for GPU-visible online descriptor heaps.
	class D3D12OnlineHeap : public TD3D12DeviceChild
	{
	public:
		D3D12OnlineHeap(ID3D12Device* device, bool bInCanLoopAround);
		virtual ~D3D12OnlineHeap() = default;

		// Check if we can reserve the requested number of slots
		bool CanReserveSlots(uint32 numSlots) const;

		// Reserve slots and return the starting slot index
		uint32 ReserveSlots(uint32 numSlotsRequested);

		// Get CPU/GPU descriptor handles for a slot
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 slot) const;
		virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 slot) const;

		// Get the underlying heap
		D3D12DescriptorHeap* GetHeap() const { return Heap.Get(); }

		virtual uint32 GetTotalSize() const { return Heap->GetDesc().NumDescriptors(); }

		// Called when the heap is full and needs to roll over
		virtual bool RollOver() = 0;

		// Called when slot wraps around to 0
		virtual void HeapLoopedAround() {}

		// Open/Close command list
		virtual void OpenCommandList() {}
		virtual void CloseCommandList() {}

	protected:
		TRefCountPtr<D3D12DescriptorHeap> Heap;
		uint32 NextSlotIndex = 0;
		uint32 FirstUsedSlot = 0;
		const bool bCanLoopAround;
	};

	// Sub-allocates blocks from the global descriptor heap, this is the default allocation strategy.
	class D3D12SubAllocatedOnlineHeap : public D3D12OnlineHeap
	{
	public:
		D3D12SubAllocatedOnlineHeap(D3D12DescriptorCache& descriptorCache, D3D12CommandContext& context);
		~D3D12SubAllocatedOnlineHeap() override;

		bool RollOver() override;
		void OpenCommandList() override;
		void CloseCommandList() override;
		uint32 GetTotalSize() const final override
		{
			return CurrentBlock ? CurrentBlock->Size : 0;
		}

		// Override to add block offset
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSlotHandle(uint32 slot) const override;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSlotHandle(uint32 slot) const override;

	private:
		bool AllocateBlock();

		D3D12DescriptorCache& DescriptorCache;
		D3D12CommandContext& Context;
		D3D12OnlineDescriptorBlock* CurrentBlock = nullptr;
	};

	// Context-local heap used as fallback when global heap is exhausted.
	class D3D12LocalOnlineHeap : public D3D12OnlineHeap
	{
	public:
		D3D12LocalOnlineHeap(D3D12DescriptorCache& descriptorCache, D3D12CommandContext& context);
		~D3D12LocalOnlineHeap() override;

		void Init(uint32 numDescriptors);

		bool RollOver() override;
		void OpenCommandList() override;
		void CloseCommandList() override;

	private:
		void RecycleSlots();

		D3D12DescriptorCache& DescriptorCache;
		D3D12CommandContext& Context;

		// Sync points for tracking GPU progress
		struct FSyncPointEntry
		{
			uint64 FenceValue = 0;
			uint32 LastSlotInUse = 0;
		};
		TQueue<FSyncPointEntry> SyncPoints;

		// Heap pool for reuse
		struct FPoolEntry
		{
			TRefCountPtr<D3D12DescriptorHeap> Heap;
			uint64 FenceValue = 0;
		};
		FPoolEntry Entry{};
		TQueue<FPoolEntry> ReclaimPool;
	};

	// Per-context descriptor cache managing online heaps.
	class D3D12DescriptorCache : public TD3D12DeviceChild
	{
	public:
		D3D12DescriptorCache(ID3D12Device* device, D3D12CommandContext& context);
		~D3D12DescriptorCache();

		// Initialize with local heap size
		void Init(uint32 numLocalViewDescriptors);

		// Open/Close command list
		void OpenCommandList();
		void CloseCommandList();

		// Get current view heap
		D3D12OnlineHeap* GetCurrentViewHeap() const { return CurrentViewHeap; }

		// Switch to context-local view heap (when global heap is exhausted)
		bool SwitchToContextLocalViewHeap();

		// Notify that heap has rolled over
		bool HeapRolledOver();

		// Set descriptor heaps on command list
		bool SetDescriptorHeaps();

	private:
		D3D12CommandContext& Context;

		D3D12OnlineHeap* CurrentViewHeap = nullptr;

		D3D12SubAllocatedOnlineHeap SubAllocatedViewHeap;
		D3D12LocalOnlineHeap* LocalViewHeap = nullptr;

		ID3D12DescriptorHeap* LastSetViewHeap = nullptr;

		uint32 NumLocalViewDescriptors = 0;
	};
}
