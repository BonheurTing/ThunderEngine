#include "D3D12UploadHeapAllocator.h"

#include <bit>

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
            D3D12_RESOURCE_STATE_GENERIC_READ,
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

    D3D12ResourceLocation D3D12SmallBlockAllocator::Allocate(uint32 size)
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

            D3D12ResourceLocation location;
            location.Resource = page->Resource.Get();
            location.GPUVirtualAddress = page->GPUBaseAddress + offset;
            location.CPUAddress = static_cast<uint8*>(page->CPUBaseAddress) + offset;
            location.OffsetInResource = offset;
            location.AllocatedSize = bucket.BlockSize;
            location.BucketIndex = bucketIndex;
            location.PageIndex = pageIndex;
            return location;
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

                D3D12ResourceLocation location;
                location.Resource = page->Resource.Get();
                location.GPUVirtualAddress = page->GPUBaseAddress + offset;
                location.CPUAddress = static_cast<uint8*>(page->CPUBaseAddress) + offset;
                location.OffsetInResource = offset;
                location.AllocatedSize = bucket.BlockSize;
                location.BucketIndex = bucketIndex;
                location.PageIndex = pageIndex;
                return location;
            }
        }

        // All existing pages are full. Allocate a new page.
        UploadHeapPage* newPage = AllocateNewPage(bucketIndex);
        if (!newPage) [[unlikely]]
        {
            D3D12ResourceLocation invalid;
            return invalid;
        }

        uint16 pageIndex = static_cast<uint16>(bucket.Pages.size());
        bucket.Pages.push_back(newPage);

        uint32 blockIndex = newPage->NextFreshBlock++;
        newPage->AllocatedBlocks++;

        uint32 offset = blockIndex * bucket.BlockSize;

        D3D12ResourceLocation location;
        location.Resource = newPage->Resource.Get();
        location.GPUVirtualAddress = newPage->GPUBaseAddress + offset;
        location.CPUAddress = static_cast<uint8*>(newPage->CPUBaseAddress) + offset;
        location.OffsetInResource = offset;
        location.AllocatedSize = bucket.BlockSize;
        location.BucketIndex = bucketIndex;
        location.PageIndex = pageIndex;
        return location;
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
            D3D12_RESOURCE_STATE_GENERIC_READ,
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

    D3D12ResourceLocation D3D12BigBlockAllocator::Allocate(uint32 size)
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

            D3D12ResourceLocation location;
            location.Resource = entry.Resource.Get();
            location.GPUVirtualAddress = entry.GPUAddress;
            location.CPUAddress = entry.CPUAddress;
            location.OffsetInResource = 0;
            location.AllocatedSize = entry.Size;
            location.BucketIndex = UINT32_MAX;
            location.PageIndex = 0;

            // We need to prevent the ComPtr from releasing the resource.
            // Store the ComPtr so it stays alive as long as the resource is in use.
            // The caller is responsible for calling Free() which will reclaim it.
            entry.Resource.Detach();

            return location;
        }

        // No pooled resource available. Create a new one.
        // Release the lock during the D3D12 API call to avoid holding it.
        guard.Unlock();

        BigBlockEntry entry = CreateCommittedUploadBuffer(alignedSize);

        if (!entry.Resource) [[unlikely]]
        {
            D3D12ResourceLocation invalid;
            return invalid;
        }

        D3D12ResourceLocation location;
        location.Resource = entry.Resource.Get();
        location.GPUVirtualAddress = entry.GPUAddress;
        location.CPUAddress = entry.CPUAddress;
        location.OffsetInResource = 0;
        location.AllocatedSize = entry.Size;
        location.BucketIndex = UINT32_MAX;
        location.PageIndex = 0;

        entry.Resource.Detach();

        return location;
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

    D3D12ResourceLocation D3D12PersistentUploadHeapAllocator::Allocate(uint32 size)
    {
        if (size == 0) [[unlikely]]
        {
            TAssertf(false, "D3D12PersistentUploadHeapAllocator::Allocate: size must be > 0.");
            return {};
        }

        if (size <= UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD)
        {
            return SmallBlockAllocator.Allocate(size);
        }
        else
        {
            return BigBlockAllocator.Allocate(size);
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

    void D3D12PersistentUploadHeapAllocator::GarbageCollect()
    {
        SmallBlockAllocator.GarbageCollect();
        BigBlockAllocator.GarbageCollect();
    }
}
