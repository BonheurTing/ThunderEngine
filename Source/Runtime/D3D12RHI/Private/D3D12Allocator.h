#pragma once

#include "CoreMinimal.h"
#include "D3D12RHICommon.h"
#include "d3dx12.h"
#include "Concurrent/Lock.h"

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

namespace Thunder
{
    // Allocation handle returned by the upload heap allocator.
    // Represents a sub-region within a larger upload buffer (small block)
    // or an entire committed resource (big block).
    struct D3D12ResourceLocation
    {
        ID3D12Resource* Resource = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;
        void* CPUAddress = nullptr;
        uint32 OffsetInResource = 0;
        uint32 AllocatedSize = 0;
        uint32 BucketIndex = UINT32_MAX;
        uint16 PageIndex = 0;

        bool IsValid() const { return Resource != nullptr; }
        void Invalidate() { Resource = nullptr; CPUAddress = nullptr; GPUVirtualAddress = 0; }
    };

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

        D3D12ResourceLocation Allocate(uint32 size);
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

        D3D12ResourceLocation Allocate(uint32 size);
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

        D3D12ResourceLocation Allocate(uint32 size);
        void Free(const D3D12ResourceLocation& allocation);

        // Call once per frame to reclaim idle resources.
        void GarbageCollect();

    private:
        D3D12SmallBlockAllocator SmallBlockAllocator;
        D3D12BigBlockAllocator BigBlockAllocator;
    };
}
