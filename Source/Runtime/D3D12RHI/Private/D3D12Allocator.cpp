#include "D3D12Allocator.h"

#include <bit>

#include "D3D12CommandContext.h"
#include "Misc/CoreGlabal.h"

namespace Thunder
{
    // ============================================================================
    // Utility
    // ============================================================================

    static inline uint32 Align256(uint32 size)
    {
        return (size + 255u) & ~255u;
    }

    static inline uint32 RoundUpPow2(uint32 v)
    {
        return std::bit_ceil(v);
    }

    // ============================================================================
    // D3D12SmallBlockAllocator
    // ============================================================================

    D3D12SmallBlockAllocator::D3D12SmallBlockAllocator(ID3D12Device* device)
        : TD3D12DeviceChild(device)
    {
        for (uint32 i = 0; i < NUM_BUCKETS; ++i)
        {
            Buckets[i].BlockSize = MIN_BLOCK_SIZE << i;
        }
    }

    D3D12SmallBlockAllocator::~D3D12SmallBlockAllocator()
    {
        for (uint32 i = 0; i < NUM_BUCKETS; ++i)
        {
            for (UploadHeapPage* page : Buckets[i].Pages)
            {
                DestroyPage(page);
            }
            Buckets[i].Pages.clear();
            Buckets[i].FreeList.clear();
        }
    }

    uint32 D3D12SmallBlockAllocator::GetBucketIndex(uint32 alignedSize)
    {
        // alignedSize is already 256-aligned and rounded up to power of 2.
        // Bucket index = log2(alignedSize) - log2(256)
        TAssert(alignedSize >= MIN_BLOCK_SIZE && alignedSize <= UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD);
        uint32 index = std::countr_zero(alignedSize) - MIN_BLOCK_SIZE_LOG2;
        TAssert(index < NUM_BUCKETS);
        return index;
    }

    uint32 D3D12SmallBlockAllocator::AlignToBucket(uint32 size)
    {
        uint32 aligned = Align256(size);
        if (aligned < MIN_BLOCK_SIZE)
        {
            aligned = MIN_BLOCK_SIZE;
        }
        return RoundUpPow2(aligned);
    }

    UploadHeapPage* D3D12SmallBlockAllocator::AllocateNewPage(uint32 bucketIndex)
    {
        UploadHeapBucket& bucket = Buckets[bucketIndex];

        constexpr D3D12_HEAP_PROPERTIES heapProps = {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(UPLOAD_HEAP_PAGE_SIZE);

        ID3D12Resource* resource = nullptr;
        HRESULT hr = GetParentDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&resource));

        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12SmallBlockAllocator: Failed to create %u byte upload heap page (HRESULT: 0x%08X).",
                     UPLOAD_HEAP_PAGE_SIZE, static_cast<uint32>(hr));
            return nullptr;
        }

        // Persistently map the page (never unmap until destruction).
        void* cpuAddress = nullptr;
        const D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read from this resource on CPU.
        hr = resource->Map(0, &readRange, &cpuAddress);
        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12SmallBlockAllocator: Failed to map upload heap page (HRESULT: 0x%08X).",
                     static_cast<uint32>(hr));
            resource->Release();
            return nullptr;
        }

        UploadHeapPage* page = new UploadHeapPage();
        page->Resource.Attach(resource);
        page->CPUBaseAddress = cpuAddress;
        page->GPUBaseAddress = resource->GetGPUVirtualAddress();
        page->PageSize = UPLOAD_HEAP_PAGE_SIZE;
        page->BlockSize = bucket.BlockSize;
        page->TotalBlocks = UPLOAD_HEAP_PAGE_SIZE / bucket.BlockSize;
        page->AllocatedBlocks = 0;
        page->NextFreshBlock = 0;
        page->UnusedFrameCount = 0;

        return page;
    }

    void D3D12SmallBlockAllocator::DestroyPage(UploadHeapPage* page)
    {
        if (page)
        {
            if (page->Resource)
            {
                page->Resource->Unmap(0, nullptr);
            }
            delete page;
        }
    }

    void* D3D12SmallBlockAllocator::Allocate(uint32 size, D3D12ResourceLocation& outLocation)
    {
        TAssert(size > 0 && size <= UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD);

        const uint32 bucketSize = AlignToBucket(size);
        const uint32 bucketIndex = GetBucketIndex(bucketSize);
        UploadHeapBucket& bucket = Buckets[bucketIndex];

        auto guard = bucket.Lock.Guard();

        // Try to reuse a freed block from the free list.
        if (!bucket.FreeList.empty())
        {
            uint32 encoded = bucket.FreeList.back();
            bucket.FreeList.pop_back();

            uint16 pageIndex = static_cast<uint16>(encoded >> 16);
            uint32 blockIndex = encoded & 0xFFFF;

            TAssertf(pageIndex < bucket.Pages.size(),
                     "D3D12SmallBlockAllocator: Invalid page index %u (bucket %u has %zu pages).",
                     pageIndex, bucketIndex, bucket.Pages.size());

            UploadHeapPage* page = bucket.Pages[pageIndex];
            page->AllocatedBlocks++;
            page->UnusedFrameCount = 0;

            uint32 offset = blockIndex * bucket.BlockSize;

            outLocation.Resource = page->Resource.Get();
            outLocation.GPUVirtualAddress = page->GPUBaseAddress + offset;
            outLocation.CPUAddress = static_cast<uint8*>(page->CPUBaseAddress) + offset;
            outLocation.OffsetInResource = offset;
            outLocation.AllocatedSize = bucket.BlockSize;
            outLocation.BucketIndex = bucketIndex;
            outLocation.PageIndex = pageIndex;
            return outLocation.CPUAddress;
        }

        // No free blocks available. Try to allocate from existing pages with fresh capacity.
        for (uint16 pageIndex = 0; pageIndex < static_cast<uint16>(bucket.Pages.size()); ++pageIndex)
        {
            UploadHeapPage* page = bucket.Pages[pageIndex];
            if (page->NextFreshBlock < page->TotalBlocks)
            {
                uint32 blockIndex = page->NextFreshBlock++;
                page->AllocatedBlocks++;
                page->UnusedFrameCount = 0;

                uint32 offset = blockIndex * bucket.BlockSize;

                outLocation.Resource = page->Resource.Get();
                outLocation.GPUVirtualAddress = page->GPUBaseAddress + offset;
                outLocation.CPUAddress = static_cast<uint8*>(page->CPUBaseAddress) + offset;
                outLocation.OffsetInResource = offset;
                outLocation.AllocatedSize = bucket.BlockSize;
                outLocation.BucketIndex = bucketIndex;
                outLocation.PageIndex = pageIndex;
                return outLocation.CPUAddress;
            }
        }

        // All existing pages are full. Allocate a new page.
        UploadHeapPage* newPage = AllocateNewPage(bucketIndex);
        if (!newPage) [[unlikely]]
        {
            outLocation.Invalidate();
            return nullptr;
        }

        uint16 pageIndex = static_cast<uint16>(bucket.Pages.size());
        bucket.Pages.push_back(newPage);

        uint32 blockIndex = newPage->NextFreshBlock++;
        newPage->AllocatedBlocks++;

        uint32 offset = blockIndex * bucket.BlockSize;

        outLocation.Resource = newPage->Resource.Get();
        outLocation.GPUVirtualAddress = newPage->GPUBaseAddress + offset;
        outLocation.CPUAddress = static_cast<uint8*>(newPage->CPUBaseAddress) + offset;
        outLocation.OffsetInResource = offset;
        outLocation.AllocatedSize = bucket.BlockSize;
        outLocation.BucketIndex = bucketIndex;
        outLocation.PageIndex = pageIndex;
        return outLocation.CPUAddress;
    }

    void D3D12SmallBlockAllocator::Free(const D3D12ResourceLocation& allocation)
    {
        TAssert(allocation.BucketIndex < NUM_BUCKETS);

        UploadHeapBucket& bucket = Buckets[allocation.BucketIndex];
        auto guard = bucket.Lock.Guard();

        TAssertf(allocation.PageIndex < bucket.Pages.size(),
                 "D3D12SmallBlockAllocator::Free: Invalid page index %u (bucket %u has %zu pages).",
                 allocation.PageIndex, allocation.BucketIndex, bucket.Pages.size());

        UploadHeapPage* page = bucket.Pages[allocation.PageIndex];
        TAssert(page->AllocatedBlocks > 0);
        page->AllocatedBlocks--;

        uint32 blockIndex = allocation.OffsetInResource / bucket.BlockSize;
        uint32 encoded = (static_cast<uint32>(allocation.PageIndex) << 16) | blockIndex;
        bucket.FreeList.push_back(encoded);
    }

    uint32 D3D12SmallBlockAllocator::GarbageCollect()
    {
        uint32 totalReleased = 0;

        for (uint32 i = 0; i < NUM_BUCKETS; ++i)
        {
            UploadHeapBucket& bucket = Buckets[i];
            auto guard = bucket.Lock.Guard();

            // Don't reclaim pages if this bucket only has one page.
            if (bucket.Pages.size() <= 1)
            {
                // Still tick the unused counter for the single page.
                if (!bucket.Pages.empty() && bucket.Pages[0]->AllocatedBlocks == 0)
                {
                    bucket.Pages[0]->UnusedFrameCount++;
                }
                continue;
            }

            // Identify fully-free pages.
            for (int32 pageIdx = static_cast<int32>(bucket.Pages.size()) - 1; pageIdx >= 0; --pageIdx)
            {
                UploadHeapPage* page = bucket.Pages[pageIdx];

                if (page->AllocatedBlocks == 0)
                {
                    page->UnusedFrameCount++;

                    if (page->UnusedFrameCount >= UPLOAD_HEAP_GC_EMPTY_PAGE_FRAMES && bucket.Pages.size() > 1)
                    {
                        // Remove all free list entries referencing this page.
                        uint16 removedPageIndex = static_cast<uint16>(pageIdx);
                        auto& freeList = bucket.FreeList;
                        for (int32 fi = static_cast<int32>(freeList.size()) - 1; fi >= 0; --fi)
                        {
                            uint16 entryPage = static_cast<uint16>(freeList[fi] >> 16);
                            if (entryPage == removedPageIndex)
                            {
                                freeList[fi] = freeList.back();
                                freeList.pop_back();
                            }
                        }

                        // Update free list entries that reference pages after the removed one.
                        for (uint32& entry : freeList)
                        {
                            uint16 entryPage = static_cast<uint16>(entry >> 16);
                            if (entryPage > removedPageIndex)
                            {
                                uint32 blockPart = entry & 0xFFFF;
                                entry = (static_cast<uint32>(entryPage - 1) << 16) | blockPart;
                            }
                        }

                        DestroyPage(page);
                        bucket.Pages.erase(bucket.Pages.begin() + pageIdx);
                        totalReleased++;
                    }
                }
                else
                {
                    page->UnusedFrameCount = 0;
                }
            }
        }

        return totalReleased;
    }

    // ============================================================================
    // D3D12BigBlockAllocator
    // ============================================================================

    D3D12BigBlockAllocator::D3D12BigBlockAllocator(ID3D12Device* device)
        : TD3D12DeviceChild(device)
    {
    }

    D3D12BigBlockAllocator::~D3D12BigBlockAllocator()
    {
        // Release all pooled resources. Unmap before release.
        for (auto& [size, entries] : FreePool)
        {
            for (auto& entry : entries)
            {
                if (entry.Resource)
                {
                    entry.Resource->Unmap(0, nullptr);
                }
            }
        }
        FreePool.clear();
    }

    BigBlockEntry D3D12BigBlockAllocator::CreateCommittedUploadBuffer(uint32 size)
    {
        constexpr D3D12_HEAP_PROPERTIES heapProps = {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

        BigBlockEntry entry;
        entry.Size = size;

        ID3D12Resource* resource = nullptr;
        HRESULT hr = GetParentDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&resource));

        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12BigBlockAllocator: Failed to create %u byte committed upload buffer (HRESULT: 0x%08X).",
                     size, static_cast<uint32>(hr));
            return entry;
        }

        const D3D12_RANGE readRange = { 0, 0 };
        void* cpuAddress = nullptr;
        hr = resource->Map(0, &readRange, &cpuAddress);
        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12BigBlockAllocator: Failed to map %u byte upload buffer (HRESULT: 0x%08X).",
                     size, static_cast<uint32>(hr));
            resource->Release();
            return entry;
        }

        entry.Resource.Attach(resource);
        entry.CPUAddress = cpuAddress;
        entry.GPUAddress = resource->GetGPUVirtualAddress();
        entry.UnusedFrameCount = 0;

        return entry;
    }

    void* D3D12BigBlockAllocator::Allocate(uint32 size, D3D12ResourceLocation& outLocation)
    {
        const uint32 alignedSize = Align256(size);

        auto guard = Lock.Guard();

        // Try to reuse a pooled resource of the same size.
        auto it = FreePool.find(alignedSize);
        if (it != FreePool.end() && !it->second.empty())
        {
            BigBlockEntry entry = std::move(it->second.back());
            it->second.pop_back();
            if (it->second.empty())
            {
                FreePool.erase(it);
            }

            outLocation.Resource = entry.Resource.Get();
            outLocation.GPUVirtualAddress = entry.GPUAddress;
            outLocation.CPUAddress = entry.CPUAddress;
            outLocation.OffsetInResource = 0;
            outLocation.AllocatedSize = entry.Size;
            outLocation.BucketIndex = UINT32_MAX;
            outLocation.PageIndex = 0;

            // Prevent ComPtr from releasing; caller owns the resource until Free().
            entry.Resource.Detach();

            return outLocation.CPUAddress;
        }

        // No pooled resource available. Create a new one.
        // Release the lock during the D3D12 API call to avoid holding it.
        guard.Unlock();

        BigBlockEntry entry = CreateCommittedUploadBuffer(alignedSize);

        if (!entry.Resource) [[unlikely]]
        {
            outLocation.Invalidate();
            return nullptr;
        }

        outLocation.Resource = entry.Resource.Get();
        outLocation.GPUVirtualAddress = entry.GPUAddress;
        outLocation.CPUAddress = entry.CPUAddress;
        outLocation.OffsetInResource = 0;
        outLocation.AllocatedSize = entry.Size;
        outLocation.BucketIndex = UINT32_MAX;
        outLocation.PageIndex = 0;

        entry.Resource.Detach();

        return outLocation.CPUAddress;
    }

    void D3D12BigBlockAllocator::Free(const D3D12ResourceLocation& allocation)
    {
        TAssert(allocation.BucketIndex == UINT32_MAX);
        TAssert(allocation.Resource != nullptr);

        BigBlockEntry entry;
        entry.Resource.Attach(allocation.Resource);
        entry.CPUAddress = allocation.CPUAddress;
        entry.GPUAddress = allocation.GPUVirtualAddress;
        entry.Size = allocation.AllocatedSize;
        entry.UnusedFrameCount = 0;

        auto guard = Lock.Guard();
        FreePool[allocation.AllocatedSize].push_back(std::move(entry));
    }

    uint32 D3D12BigBlockAllocator::GarbageCollect()
    {
        auto guard = Lock.Guard();

        uint32 totalReleased = 0;

        for (auto it = FreePool.begin(); it != FreePool.end(); )
        {
            auto& entries = it->second;
            for (int32 i = static_cast<int32>(entries.size()) - 1; i >= 0; --i)
            {
                entries[i].UnusedFrameCount++;
                if (entries[i].UnusedFrameCount >= UPLOAD_HEAP_GC_BIG_BLOCK_FRAMES)
                {
                    entries[i].Resource->Unmap(0, nullptr);
                    entries[i] = std::move(entries.back());
                    entries.pop_back();
                    totalReleased++;
                }
            }

            if (entries.empty())
            {
                it = FreePool.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return totalReleased;
    }

    // ============================================================================
    // D3D12PersistentUploadHeapAllocator
    // ============================================================================

    D3D12PersistentUploadHeapAllocator::D3D12PersistentUploadHeapAllocator(ID3D12Device* device)
        : TD3D12DeviceChild(device)
        , SmallBlockAllocator(device)
        , BigBlockAllocator(device)
    {
    }

    D3D12PersistentUploadHeapAllocator::~D3D12PersistentUploadHeapAllocator()
    {
    }

    void* D3D12PersistentUploadHeapAllocator::Allocate(uint32 size, D3D12ResourceLocation& outLocation)
    {
        if (size == 0) [[unlikely]]
        {
            TAssertf(false, "D3D12PersistentUploadHeapAllocator::Allocate: size must be > 0.");
            outLocation.Invalidate();
            return nullptr;
        }

        if (size <= UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD)
        {
            return SmallBlockAllocator.Allocate(size, outLocation);
        }
        else
        {
            return BigBlockAllocator.Allocate(size, outLocation);
        }
    }

    void D3D12PersistentUploadHeapAllocator::Free(const D3D12ResourceLocation& allocation)
    {
        if (!allocation.IsValid()) [[unlikely]]
        {
            return;
        }

        if (allocation.BucketIndex != UINT32_MAX)
        {
            SmallBlockAllocator.Free(allocation);
        }
        else
        {
            BigBlockAllocator.Free(allocation);
        }
    }

    void D3D12PersistentUploadHeapAllocator::DeferredFree(const D3D12ResourceLocation& allocation, uint32 currentFrameNumber)
    {
        if (!allocation.IsValid()) [[unlikely]]
        {
            return;
        }

        uint32 index = currentFrameNumber % MAX_FRAME_LAG;
        auto guard = DeferredFreeLock.Guard();
        DeferredFreeQueue[index].push_back(allocation);
    }

    void D3D12PersistentUploadHeapAllocator::GarbageCollect(uint32 currentFrameNumber)
    {
        // Flush the deferred free queue from MAX_FRAME_LAG frames ago.
        // By now the GPU has finished reading from these locations.
        {
            uint32 flushIndex = (currentFrameNumber + 1) % MAX_FRAME_LAG;
            auto guard = DeferredFreeLock.Guard();
            for (const D3D12ResourceLocation& location : DeferredFreeQueue[flushIndex])
            {
                Free(location);
            }
            DeferredFreeQueue[flushIndex].clear();
        }

        SmallBlockAllocator.GarbageCollect();
        BigBlockAllocator.GarbageCollect();
    }

    // ============================================================================
    // D3D12TransientUploadHeapAllocator
    // ============================================================================

    D3D12TransientUploadHeapAllocator::D3D12TransientUploadHeapAllocator(ID3D12Device* device)
        : TD3D12DeviceChild(device)
    {
        for (uint32 i = 0; i < MAX_FRAME_LAG; ++i)
        {
            TransientUploadPage* page = CreatePage();
            TAssertf(page != nullptr, "D3D12TransientUploadHeapAllocator: Failed to create primary page for frame %u.", i);
            FrameHeaps[i].PrimaryPage = page;
            FrameHeaps[i].CurrentPage = page;
            FrameHeaps[i].CurrentOffset = 0;
        }
    }

    D3D12TransientUploadHeapAllocator::~D3D12TransientUploadHeapAllocator()
    {
        for (uint32 i = 0; i < MAX_FRAME_LAG; ++i)
        {
            TransientFrameHeap& heap = FrameHeaps[i];
            for (TransientUploadPage* page : heap.OverflowPages)
            {
                DestroyPage(page);
            }
            heap.OverflowPages.clear();
            DestroyPage(heap.PrimaryPage);
            heap.PrimaryPage = nullptr;
            heap.CurrentPage = nullptr;
        }

        for (TransientUploadPage* page : PagePool)
        {
            DestroyPage(page);
        }
        PagePool.clear();
    }

    TransientUploadPage* D3D12TransientUploadHeapAllocator::CreatePage()
    {
        constexpr D3D12_HEAP_PROPERTIES heapProps = {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN, 1, 1
        };
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(TRANSIENT_HEAP_PAGE_SIZE);

        ID3D12Resource* resource = nullptr;
        HRESULT hr = GetParentDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&resource));

        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12TransientUploadHeapAllocator: Failed to create %u byte upload page (HRESULT: 0x%08X).",
                     TRANSIENT_HEAP_PAGE_SIZE, static_cast<uint32>(hr));
            return nullptr;
        }

        void* cpuAddress = nullptr;
        const D3D12_RANGE readRange = { 0, 0 };
        hr = resource->Map(0, &readRange, &cpuAddress);
        if (FAILED(hr)) [[unlikely]]
        {
            TAssertf(false, "D3D12TransientUploadHeapAllocator: Failed to map upload page (HRESULT: 0x%08X).",
                     static_cast<uint32>(hr));
            resource->Release();
            return nullptr;
        }

        TransientUploadPage* page = new TransientUploadPage();
        page->Resource.Attach(resource);
        page->CPUBaseAddress = cpuAddress;
        page->GPUBaseAddress = resource->GetGPUVirtualAddress();
        page->PageSize = TRANSIENT_HEAP_PAGE_SIZE;
        return page;
    }

    void D3D12TransientUploadHeapAllocator::DestroyPage(TransientUploadPage* page)
    {
        if (page)
        {
            if (page->Resource)
            {
                page->Resource->Unmap(0, nullptr);
            }
            delete page;
        }
    }

    TransientUploadPage* D3D12TransientUploadHeapAllocator::AcquirePage()
    {
        if (!PagePool.empty())
        {
            TransientUploadPage* page = PagePool.back();
            PagePool.pop_back();
            PagePoolIdleFrameCount = 0;
            return page;
        }
        return CreatePage();
    }

    void D3D12TransientUploadHeapAllocator::ReleasePage(TransientUploadPage* page)
    {
        PagePool.push_back(page);
    }

    void* D3D12TransientUploadHeapAllocator::Allocate(uint32 size, D3D12ResourceLocation& outLocation)
    {
        const uint32 alignedSize = Align256(size);
        TAssertf(alignedSize <= TRANSIENT_HEAP_PAGE_SIZE,
                 "D3D12TransientUploadHeapAllocator: Allocation size %u (aligned %u) exceeds page size %u.",
                 size, alignedSize, TRANSIENT_HEAP_PAGE_SIZE);

        // Determine current frame heap from render thread frame number
        uint32 frameIndex = GFrameState->FrameNumberRenderThread.load(std::memory_order_acquire) % MAX_FRAME_LAG;
        TransientFrameHeap& heap = FrameHeaps[frameIndex];

        // Fast path: fits in current page
        if (heap.CurrentOffset + alignedSize <= TRANSIENT_HEAP_PAGE_SIZE)
        {
            TransientUploadPage* page = heap.CurrentPage;
            uint32 offset = heap.CurrentOffset;

            outLocation.Resource = page->Resource.Get();
            outLocation.GPUVirtualAddress = page->GPUBaseAddress + offset;
            outLocation.CPUAddress = static_cast<uint8*>(page->CPUBaseAddress) + offset;
            outLocation.OffsetInResource = offset;
            outLocation.AllocatedSize = alignedSize;
            outLocation.BucketIndex = UINT32_MAX;
            outLocation.PageIndex = 0;

            heap.CurrentOffset += alignedSize;
            return outLocation.CPUAddress;
        }

        // Slow path: current page full, acquire a new overflow page
        TransientUploadPage* newPage = AcquirePage();
        if (!newPage) [[unlikely]]
        {
            TAssertf(false, "D3D12TransientUploadHeapAllocator: Failed to acquire overflow page.");
            return nullptr;
        }

        heap.OverflowPages.push_back(newPage);
        heap.CurrentPage = newPage;
        heap.CurrentOffset = alignedSize;

        outLocation.Resource = newPage->Resource.Get();
        outLocation.GPUVirtualAddress = newPage->GPUBaseAddress;
        outLocation.CPUAddress = newPage->CPUBaseAddress;
        outLocation.OffsetInResource = 0;
        outLocation.AllocatedSize = alignedSize;
        outLocation.BucketIndex = UINT32_MAX;
        outLocation.PageIndex = 0;

        return outLocation.CPUAddress;
    }

    void D3D12TransientUploadHeapAllocator::FrameFlip(uint32 currentFrameNumber)
    {
        // The next slot in the ring corresponds to data from MAX_FRAME_LAG frames ago,
        // which the GPU has finished reading by now.
        uint32 nextIndex = (currentFrameNumber + 1) % MAX_FRAME_LAG;
        TransientFrameHeap& heap = FrameHeaps[nextIndex];

        // Recycle overflow pages back to the pool
        for (TransientUploadPage* page : heap.OverflowPages)
        {
            ReleasePage(page);
        }
        heap.OverflowPages.clear();

        // Reset to primary page
        heap.CurrentPage = heap.PrimaryPage;
        heap.CurrentOffset = 0;

        // GC: if pool pages have been idle for too long, release them
        if (!PagePool.empty())
        {
            PagePoolIdleFrameCount++;
            if (PagePoolIdleFrameCount >= TRANSIENT_HEAP_GC_IDLE_FRAMES)
            {
                for (TransientUploadPage* page : PagePool)
                {
                    DestroyPage(page);
                }
                PagePool.clear();
                PagePoolIdleFrameCount = 0;
            }
        }
        else
        {
            PagePoolIdleFrameCount = 0;
        }
    }
}
