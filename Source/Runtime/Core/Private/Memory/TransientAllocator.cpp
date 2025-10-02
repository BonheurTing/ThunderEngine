#pragma optimize("", off)
#include "Memory/TransientAllocator.h"
#include <algorithm>
#include <cassert>

namespace Thunder
{

    // MemoryBlock implementation
    TransientAllocator::MemoryBlock::MemoryBlock(size_t size)
        : Size(size), Used(0)
    {
        Memory = std::malloc(size);
// #if !SHIPPING
        memset(Memory, 0xce, Size);
// #endif
        assert(Memory != nullptr && "Failed to allocate memory block");
    }

    TransientAllocator::MemoryBlock::~MemoryBlock()
    {
        if (Memory)
        {
            std::free(Memory);
            Memory = nullptr;
        }
    }

    void* TransientAllocator::MemoryBlock::Allocate(size_t size, uint32 alignment)
    {
        if (size == 0) return nullptr;

        size_t alignedOffset = AlignUp(Used, alignment);
        size_t alignedSize = AlignUp(size, alignment);

        if (alignedOffset + alignedSize > Size)
        {
            return nullptr; // Not enough space
        }

        void* result = static_cast<char*>(Memory) + alignedOffset;
// #if !SHIPPING
        memset(result, 0, alignedSize);
// #endif
        Used = alignedOffset + alignedSize;
        return result;
    }

    void TransientAllocator::MemoryBlock::Reset()
    {
        Used = 0;
// #if !SHIPPING
        memset(Memory, 0xce, Size);
// #endif
    }

    // TransientAllocator implementation
    TransientAllocator::TransientAllocator()
        : CurrentBlockIndex(0), FrameCount(0), MaxBlocksUsedInPeriod(0)
    {
        // Create initial block
        Blocks.push_back(new MemoryBlock(TRANSIENT_ALLOCATOR_BLOCK_SIZE));
    }

    TransientAllocator::~TransientAllocator()
    {
        // Free all large allocations
        for (const auto& pair : LargeBlocks)
        {
            std::free(pair.second.Memory);
        }
        LargeBlocks.clear();

        for (MemoryBlock* block : Blocks)
        {
            delete block;
        }
        Blocks.clear();
    }

    void* TransientAllocator::Allocate(size_t size, uint32 alignment)
    {
        if (size == 0) return nullptr;

        // For large allocations, use direct malloc
        if (size >= TRANSIENT_ALLOCATOR_LARGE_ALLOCATION_THRESHOLD)
        {
            return AllocateLarge(size);
        }

        // Try to allocate from current block
        MemoryBlock* currentBlock = GetOrCreateCurrentBlock();
        void* result = currentBlock->Allocate(size, alignment);

        if (result == nullptr)
        {
            // Current block is full, try to create a new one
            size_t blockSize = std::max(static_cast<size_t>(TRANSIENT_ALLOCATOR_BLOCK_SIZE), AlignUp(size, alignment));
            Blocks.push_back(new MemoryBlock(blockSize));
            CurrentBlockIndex = Blocks.size() - 1;

            currentBlock = Blocks[CurrentBlockIndex];
            result = currentBlock->Allocate(size, alignment);

            assert(result != nullptr && "Failed to allocate from new block");
        }

// #if !SHIPPING
        memset(result, 0xce, size);
// #endif

        return result;
    }

    void* TransientAllocator::AllocateLarge(size_t size)
    {
        // First try to find a reusable allocation
        void* reusedMemory = FindReusableLargeAllocation(size);
        if (reusedMemory != nullptr)
        {
            return reusedMemory;
        }

        // No reusable allocation found, create new one
        void* memory = std::malloc(size);
        assert(memory != nullptr && "Failed to allocate large memory");
// #if !SHIPPING
        memset(memory, 0xce, size);
// #endif

        LargeBlocks.emplace(size, LargeAllocation(memory, size));
        return memory;
    }

    void TransientAllocator::FreeAll()
    {
        // Mark all large allocations as unused and increment frame count
        for (auto& pair : LargeBlocks)
        {
            LargeAllocation& allocation = pair.second;
            if (allocation.InUse)
            {
                allocation.InUse = false;
                allocation.UnusedFrameCount = 0;
            }
            else
            {
                allocation.UnusedFrameCount++;
            }
        }

        // Clean up expired large allocations
        CleanupExpiredLargeAllocations();

        // Track maximum blocks used in this period
        size_t currentBlocksUsed = CurrentBlockIndex + 1;
        MaxBlocksUsedInPeriod = std::max(currentBlocksUsed, MaxBlocksUsedInPeriod);

        // Check if it's time to cleanup excess blocks
        FrameCount++;
        if (FrameCount > TRANSIENT_ALLOCATOR_BLOCK_USAGE_CHECK_FRAMES)
        {
            // If we have more blocks than the maximum we want to keep
            if (Blocks.size() - MaxBlocksUsedInPeriod > TRANSIENT_ALLOCATOR_FREE_BLOCKS_BIAS)
            {
                // Free excess blocks
                for (size_t i = Blocks.size()-1; i >= MaxBlocksUsedInPeriod; i--)
                {
                    delete Blocks[i];
                }
                Blocks.resize(MaxBlocksUsedInPeriod);
            }

            // Reset the max blocks counter for next period
            MaxBlocksUsedInPeriod = 0;
        }

        // Reset all blocks without freeing them
        for (MemoryBlock* block : Blocks)
        {
            block->Reset();
        }

        // Reset to first block
        CurrentBlockIndex = 0;
    }

    TransientAllocator::MemoryBlock* TransientAllocator::GetOrCreateCurrentBlock()
    {
        if (CurrentBlockIndex >= Blocks.size())
        {
            Blocks.push_back(new MemoryBlock(TRANSIENT_ALLOCATOR_BLOCK_SIZE));
            CurrentBlockIndex = Blocks.size() - 1;
        }

        return Blocks[CurrentBlockIndex];
    }

    size_t TransientAllocator::GetTotalAllocatedMemory() const
    {
        size_t total = 0;

        // Add block memory
        for (const MemoryBlock* block : Blocks)
        {
            total += block->Size;
        }

        // Add large allocations
        for (const auto& pair : LargeBlocks)
        {
            total += pair.second.Size;
        }

        return total;
    }

    size_t TransientAllocator::GetTotalUsedMemory() const
    {
        size_t total = 0;

        // Add used memory from blocks
        for (const MemoryBlock* block : Blocks)
        {
            total += block->Used;
        }

        // Add active large allocations
        for (const auto& pair : LargeBlocks)
        {
            if (pair.second.InUse)
            {
                total += pair.second.Size;
            }
        }

        return total;
    }

    void* TransientAllocator::FindReusableLargeAllocation(size_t size)
    {
        auto range = LargeBlocks.equal_range(size);
        for (auto it = range.first; it != range.second; ++it)
        {
            LargeAllocation& allocation = it->second;
            if (!allocation.InUse)
            {
                allocation.InUse = true;
                allocation.UnusedFrameCount = 0;
                return allocation.Memory;
            }
        }
        return nullptr;
    }

    void TransientAllocator::CleanupExpiredLargeAllocations()
    {
        auto it = LargeBlocks.begin();
        while (it != LargeBlocks.end())
        {
            const LargeAllocation& allocation = it->second;
            if (!allocation.InUse && allocation.UnusedFrameCount >= TRANSIENT_ALLOCATOR_LARGE_FREE_FRAMES_THRESHOLD)
            {
                std::free(allocation.Memory);
                it = LargeBlocks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    size_t TransientAllocator::GetActiveLargeAllocationCount() const
    {
        size_t count = 0;
        for (const auto& pair : LargeBlocks)
        {
            if (pair.second.InUse)
            {
                count++;
            }
        }
        return count;
    }

    size_t TransientAllocator::AlignUp(size_t value, size_t alignment)
    {
        if (alignment == 0) alignment = 1;
        return (value + alignment - 1) & ~(alignment - 1);
    }
}