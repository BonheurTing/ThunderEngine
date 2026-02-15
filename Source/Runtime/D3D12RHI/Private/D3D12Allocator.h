#pragma once

#include "Concurrent/Lock.h"
#include "D3D12Resource.h"
#include "D3D12RHICommon.h"

#ifndef UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD
#define UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD (64 * 1024)       // 64KB: SmallBlock/BigBlock boundary
#endif

#ifndef UPLOAD_HEAP_PAGE_SIZE
#define UPLOAD_HEAP_PAGE_SIZE (2 * 1024 * 1024)             // 2MB per page
#endif

#ifndef UPLOAD_HEAP_GC_EMPTY_PAGE_FRAMES
#define UPLOAD_HEAP_GC_EMPTY_PAGE_FRAMES 120                // Frames before reclaiming empty pages
#endif

#ifndef UPLOAD_HEAP_GC_BIG_BLOCK_FRAMES
#define UPLOAD_HEAP_GC_BIG_BLOCK_FRAMES 60                  // Frames before reclaiming idle big blocks
#endif

#ifndef TRANSIENT_HEAP_PAGE_SIZE
#define TRANSIENT_HEAP_PAGE_SIZE (64 * 1024)                // 64KB per transient page
#endif

#ifndef TRANSIENT_HEAP_GC_IDLE_FRAMES
#define TRANSIENT_HEAP_GC_IDLE_FRAMES 60                    // Frames before reclaiming idle pool pages
#endif

namespace Thunder
{
    // A 2MB upload buffer page, sub-divided into equal-sized blocks.
    struct UploadHeapPage
    {
        ComPtr<ID3D12Resource> Resource;
        void* CPUBaseAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS GPUBaseAddress = 0;
        uint32 PageSize = 0;
        uint32 BlockSize = 0;
        uint32 TotalBlocks = 0;
        uint32 AllocatedBlocks = 0;
        uint32 NextFreshBlock = 0;      // Next never-allocated block index (for initial linear allocation)
        uint32 UnusedFrameCount = 0;    // Frames since page became fully free (for GC)
    };

    // A bucket manages pages of the same block size.
    struct UploadHeapBucket
    {
        uint32 BlockSize = 0;
        TArray<UploadHeapPage*> Pages;
        TArray<uint32> FreeList;        // Encoded as (pageIndex << 16) | blockIndexInPage
        SpinLock Lock;
    };

    // Sub-allocates small blocks (<=64KB) from pooled 2MB upload buffer pages.
    // Uses power-of-2 bucketing with per-bucket free lists.
    class D3D12SmallBlockAllocator : public TD3D12DeviceChild
    {
    public:
        D3D12SmallBlockAllocator(ID3D12Device* device);
        ~D3D12SmallBlockAllocator() override;

        // Allocate a block. Returns the CPU mapped address, or nullptr on failure.
        void* Allocate(uint32 size, D3D12ResourceLocation& outLocation);
        void Free(const D3D12ResourceLocation& allocation);

        // Reclaim fully-free pages that have been idle for a while.
        // Returns number of pages released.
        uint32 GarbageCollect();

        static constexpr uint32 NUM_BUCKETS = 9;    // 256B, 512B, 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB
        static constexpr uint32 MIN_BLOCK_SIZE = 256;
        static constexpr uint32 MIN_BLOCK_SIZE_LOG2 = 8;

    private:
        static uint32 GetBucketIndex(uint32 alignedSize);
        static uint32 AlignToBucket(uint32 size);
        UploadHeapPage* AllocateNewPage(uint32 bucketIndex);
        void DestroyPage(UploadHeapPage* page);

        UploadHeapBucket Buckets[NUM_BUCKETS];
    };

    // A pooled committed resource entry for big block allocation.
    struct BigBlockEntry
    {
        ComPtr<ID3D12Resource> Resource;
        void* CPUAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = 0;
        uint32 Size = 0;
        uint32 UnusedFrameCount = 0;
    };

    // Allocates big blocks (>64KB) as individual committed resources.
    // Pools freed resources by size for reuse.
    class D3D12BigBlockAllocator : public TD3D12DeviceChild
    {
    public:
        D3D12BigBlockAllocator(ID3D12Device* device);
        ~D3D12BigBlockAllocator() override;

        // Allocate a committed upload buffer. Returns the CPU mapped address, or nullptr on failure.
        void* Allocate(uint32 size, D3D12ResourceLocation& outLocation);
        void Free(const D3D12ResourceLocation& allocation);

        // Reclaim pooled resources that have been idle for a while.
        // Returns number of resources released.
        uint32 GarbageCollect();

    private:
        BigBlockEntry CreateCommittedUploadBuffer(uint32 size);

        // Free pool: key = aligned size, value = reusable resources
        TMap<uint32, TArray<BigBlockEntry>> FreePool;
        SpinLock Lock;
    };

    // Top-level persistent upload heap allocator for multi-frame uniform buffers.
    // Routes allocations to SmallBlockAllocator or BigBlockAllocator based on size.
    class D3D12PersistentUploadHeapAllocator : public TD3D12DeviceChild
    {
    public:
        D3D12PersistentUploadHeapAllocator(ID3D12Device* device);
        ~D3D12PersistentUploadHeapAllocator() override;

        // Allocate upload heap memory. Returns the CPU mapped address, or nullptr on failure.
        void* Allocate(uint32 size, D3D12ResourceLocation& outLocation);

        // Immediately free a location back to the allocator pool.
        void Free(const D3D12ResourceLocation& allocation);

        // Queue a location for deferred freeing. The location will be freed after MAX_FRAME_LAG frames
        // to ensure the GPU is no longer reading from it.
        void DeferredFree(const D3D12ResourceLocation& allocation, uint32 currentFrameNumber);

        // Call once per frame to reclaim idle resources and flush matured deferred frees.
        void GarbageCollect(uint32 currentFrameNumber);

    private:
        D3D12SmallBlockAllocator SmallBlockAllocator;
        D3D12BigBlockAllocator BigBlockAllocator;

        // Deferred free queue: locations are held for MAX_FRAME_LAG frames before being freed.
        TArray<D3D12ResourceLocation> DeferredFreeQueue[MAX_FRAME_LAG];
        SpinLock DeferredFreeLock;
    };

    // A lightweight upload buffer page for transient (single-frame) allocations.
    struct TransientUploadPage
    {
        ComPtr<ID3D12Resource> Resource;
        void* CPUBaseAddress = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS GPUBaseAddress = 0;
        uint32 PageSize = 0;
    };

    // Per-frame page group: a primary page (always resident) plus overflow pages on demand.
    struct TransientFrameHeap
    {
        TransientUploadPage* PrimaryPage = nullptr;
        TArray<TransientUploadPage*> OverflowPages;
        TransientUploadPage* CurrentPage = nullptr;
        uint32 CurrentOffset = 0;
    };

    // Transient upload heap allocator for single-frame / single-draw uniform buffers.
    // Uses linear bump allocation within per-frame page groups (ring of MAX_FRAME_LAG frames).
    // Primary page is always resident; overflow pages are acquired on demand and recycled via a pool.
    class D3D12TransientUploadHeapAllocator : public TD3D12DeviceChild
    {
    public:
        D3D12TransientUploadHeapAllocator(ID3D12Device* device);
        ~D3D12TransientUploadHeapAllocator() override;

        // Linear bump allocation. Returns the CPU mapped address, or nullptr on failure.
        void* Allocate(uint32 size, D3D12ResourceLocation& outLocation);

        // Call once per frame to recycle the next frame's pages and advance the ring.
        void FrameFlip(uint32 currentFrameNumber);

    private:
        TransientUploadPage* CreatePage();
        void DestroyPage(TransientUploadPage* page);
        TransientUploadPage* AcquirePage();
        void ReleasePage(TransientUploadPage* page);

        TransientFrameHeap FrameHeaps[MAX_FRAME_LAG];
        TArray<TransientUploadPage*> PagePool;
        uint32 PagePoolIdleFrameCount = 0;
    };
}
