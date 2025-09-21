#pragma once

#include "MemoryBase.h"
#include <vector>
#include <unordered_map>
#include <cstdlib>

#ifndef TRANSIENT_ALLOCATOR_LARGE_ALLOCATION_THRESHOLD
#define TRANSIENT_ALLOCATOR_LARGE_ALLOCATION_THRESHOLD (1024 * 1024)  // 1MB threshold
#endif

#ifndef TRANSIENT_ALLOCATOR_BLOCK_SIZE
#define TRANSIENT_ALLOCATOR_BLOCK_SIZE (4 * 1024 * 1024)  // 4MB per block
#endif

#ifndef TRANSIENT_ALLOCATOR_LARGE_FREE_FRAMES_THRESHOLD
#define TRANSIENT_ALLOCATOR_LARGE_FREE_FRAMES_THRESHOLD 3  // Free large allocations after 3 unused frames
#endif

#ifndef TRANSIENT_ALLOCATOR_BLOCK_USAGE_CHECK_FRAMES
#define TRANSIENT_ALLOCATOR_BLOCK_USAGE_CHECK_FRAMES 60  // Check block usage every N frames
#endif

#ifndef TRANSIENT_ALLOCATOR_FREE_BLOCKS_BIAS
#define TRANSIENT_ALLOCATOR_FREE_BLOCKS_BIAS 10  // Bias of most blocks when usage check
#endif

namespace Thunder
{
    class CORE_API TransientAllocator
    {
    private:
        struct MemoryBlock
        {
            void* Memory;
            size_t Size;
            size_t Used;

            MemoryBlock(size_t size);
            ~MemoryBlock();

            void* Allocate(size_t size, uint32 alignment);
            void Reset();
            size_t GetAvailableSpace() const { return Size - Used; }
        };

        struct LargeAllocation
        {
            void* Memory;
            size_t Size;
            uint32 UnusedFrameCount;
            bool InUse;

            LargeAllocation(void* memory, size_t size)
                : Memory(memory), Size(size), UnusedFrameCount(0), InUse(true) {}
        };

    public:
        TransientAllocator();
        ~TransientAllocator();

        void* Allocate(size_t size, uint32 alignment = 8);

        template<typename T>
        T* Allocate(size_t count = 1)
        {
            return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
        }

        void FreeAll();

        size_t GetTotalAllocatedMemory() const;
        size_t GetTotalUsedMemory() const;
        size_t GetBlockCount() const { return Blocks.size(); }
        size_t GetLargeAllocationCount() const { return LargeBlocks.size(); }
        size_t GetActiveLargeAllocationCount() const;

    private:
        std::vector<MemoryBlock*> Blocks;
        std::unordered_multimap<size_t, LargeAllocation> LargeBlocks;
        size_t CurrentBlockIndex;
        size_t FrameCount;
        size_t MaxBlocksUsedInPeriod;

        MemoryBlock* GetOrCreateCurrentBlock();
        void* AllocateLarge(size_t size);
        void* FindReusableLargeAllocation(size_t size);
        void CleanupExpiredLargeAllocations();

        static size_t AlignUp(size_t value, size_t alignment);
    };

    extern CORE_API TransientAllocator* GTransientAllocator;
}