# Persistent Upload Heap Allocator 设计方案

## 1. 背景与动机

当前 `RHICreateConstantBuffer` 每次都调用 `CreateCommittedResource`，为每个 uniform buffer 创建独立的 D3D12 heap + resource。对于 multi-frame persistent uniform buffer（如 per-object transform、material 参数等长期存在的 UB），这种方式存在以下问题：

- **分配开销大**：`CreateCommittedResource` 是重量级 API 调用
- **显存碎片化**：大量小 resource 导致显存碎片
- **管理复杂**：每个 UB 持有独立的 `ComPtr<ID3D12Resource>`

## 2. 设计目标

实现 `D3D12PersistentUploadHeapAllocator`，用于 multi-frame uniform buffer 的显存分配。根据申请大小分为两级：

- **SmallBlockAllocator**：≤ 64KB，使用分桶 + FreeList 从预分配的 2MB page 中 sub-allocate
- **BigBlockAllocator**：> 64KB，直接使用 `CreateCommittedResource`

## 3. 总体架构

```
D3D12PersistentUploadHeapAllocator
├── SmallBlockAllocator       （≤ 64KB）
│   ├── Bucket[0]  = 256B
│   ├── Bucket[1]  = 512B
│   ├── Bucket[2]  = 1KB
│   ├── Bucket[3]  = 2KB
│   ├── Bucket[4]  = 4KB
│   ├── Bucket[5]  = 8KB
│   ├── Bucket[6]  = 16KB
│   ├── Bucket[7]  = 32KB
│   └── Bucket[8]  = 64KB
│       └── Page Pool（每个 page 2MB upload buffer）
│           └── FreeList（空闲 block 链表）
└── BigBlockAllocator         （> 64KB）
    └── FreeList（空闲 committed resource 链表）
```

## 4. 核心数据结构

### 4.1 分配结果

```cpp
// 分配器返回的 handle，替代原来的 ID3D12Resource*
struct D3D12ResourceLocation
{
    ID3D12Resource* Resource;                   // 所属的 upload buffer resource
    D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress; // GPU 虚拟地址 = resource base + offset
    void* CPUAddress;                            // CPU 映射地址 = resource base map + offset
    uint32 OffsetInResource;                     // 在 resource 内的偏移
    uint32 AllocatedSize;                        // 实际分配大小（对齐后）
    uint32 BucketIndex;                          // 所属桶索引（SmallBlock 用，BigBlock 为 UINT32_MAX）
    uint16 PageIndex;                            // 所属 page 索引（SmallBlock 用）
    bool IsSmallBlock;                           // 标记来源
};
```

### 4.2 SmallBlockAllocator

#### Page（2MB Upload Buffer）

```cpp
struct UploadHeapPage
{
    ComPtr<ID3D12Resource> Resource;     // 2MB upload buffer (CreateCommittedResource)
    void* CPUBaseAddress;                // Map 后的 CPU 基地址（常驻映射，不 Unmap）
    D3D12_GPU_VIRTUAL_ADDRESS GPUBaseAddress; // GPU 基地址
    uint32 PageSize;                     // 2MB
    uint32 BlockSize;                    // 该 page 中每个 block 的大小（由所属桶决定）
    uint32 TotalBlocks;                  // 该 page 可容纳的 block 总数 = PageSize / BlockSize
    uint32 AllocatedBlocks;             // 已分配的 block 数（用于判断 page 是否可回收）
};
```

#### Bucket（桶）

```cpp
struct UploadHeapBucket
{
    uint32 BlockSize;                    // 该桶的 block 大小（256, 512, 1024, ...）
    TArray<UploadHeapPage*> Pages;      // 该桶下的所有 page
    TArray<uint32> FreeList;             // 空闲 block 索引列表（编码: pageIndex << 16 | blockIndex）
    SpinLock Lock;                       // 线程安全锁（per-bucket 粒度）
};
```

FreeList 中每个条目编码为 `(pageIndex << 16) | blockIndexInPage`，通过 pageIndex 找到 page，通过 blockIndex 计算 offset。

#### SmallBlockAllocator 主体

```cpp
class D3D12SmallBlockAllocator
{
public:
    D3D12ResourceLocation Allocate(uint32 size);
    void Free(const D3D12ResourceLocation& allocation);

private:
    static constexpr uint32 NUM_BUCKETS = 9;                    // 256B ~ 64KB
    static constexpr uint32 PAGE_SIZE = 2 * 1024 * 1024;        // 2MB
    static constexpr uint32 SMALL_BLOCK_THRESHOLD = 64 * 1024;  // 64KB

    UploadHeapBucket Buckets[NUM_BUCKETS];

    uint32 GetBucketIndex(uint32 size) const;       // size -> bucket index
    UploadHeapPage* AllocateNewPage(uint32 bucketIndex);
    ID3D12Device* Device;
};
```

#### 桶索引映射

| Bucket Index | Block Size | Blocks per 2MB Page |
|:---:|:---:|:---:|
| 0 | 256 B | 8192 |
| 1 | 512 B | 4096 |
| 2 | 1 KB | 2048 |
| 3 | 2 KB | 1024 |
| 4 | 4 KB | 512 |
| 5 | 8 KB | 256 |
| 6 | 16 KB | 128 |
| 7 | 32 KB | 64 |
| 8 | 64 KB | 32 |

`GetBucketIndex(size)`：将 256B 对齐后的 size 向上取到最近的 2 的幂次，然后计算 `log2(alignedSize) - 8`（因为最小桶是 256 = 2^8）。

### 4.3 BigBlockAllocator

```cpp
struct BigBlockEntry
{
    ComPtr<ID3D12Resource> Resource;
    void* CPUAddress;
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress;
    uint32 Size;
};

class D3D12BigBlockAllocator
{
public:
    D3D12ResourceLocation Allocate(uint32 size);
    void Free(const D3D12ResourceLocation& allocation);

private:
    // 按 size 分组的空闲 committed resource 列表
    // key = aligned size, value = 可复用的 resource 列表
    TMap<uint32, TArray<BigBlockEntry>> FreePool;
    TArray<BigBlockEntry> ActiveAllocations;  // 当前活跃的分配
    SpinLock Lock;
    ID3D12Device* Device;
};
```

BigBlock 复用逻辑：释放时不立即销毁 resource，而是放入 FreePool。下次分配相同 size 时优先从 FreePool 取。FreePool 中超过 N 帧未使用的 resource 才真正销毁。

### 4.4 顶层 Allocator

```cpp
class D3D12PersistentUploadHeapAllocator
{
public:
    D3D12PersistentUploadHeapAllocator(ID3D12Device* device);
    ~D3D12PersistentUploadHeapAllocator();

    // 分配
    D3D12ResourceLocation Allocate(uint32 size);

    // 释放（立即归还到 free list，不需要 deferred delete，因为调用方已保证 GPU 不再使用）
    void Free(const D3D12ResourceLocation& allocation);

    // 每帧调用，清理长期未使用的空闲 page / big block
    void GarbageCollect();

private:
    D3D12SmallBlockAllocator SmallBlockAllocator;
    D3D12BigBlockAllocator BigBlockAllocator;

    static constexpr uint32 SMALL_BLOCK_THRESHOLD = 64 * 1024; // 64KB
};
```

## 5. 分配流程

### 5.1 SmallBlock 分配

```
Allocate(size)
  → Align256(size)
  → GetBucketIndex(alignedSize)
  → Lock bucket
  → if FreeList 非空:
      → 从 FreeList 弹出一个 block index
      → 解码 pageIndex + blockIndex
      → 计算 offset = blockIndex * blockSize
      → 返回 {page.Resource, page.GPUBase + offset, page.CPUBase + offset, ...}
  → else:
      → 遍历 pages 找到有空闲容量的 page
      → 如果没有，AllocateNewPage()
      → 从 page 分配下一个 block
      → 返回 allocation
  → Unlock bucket
```

### 5.2 SmallBlock 释放

```
Free(allocation)
  → Lock bucket[allocation.BucketIndex]
  → 编码 (allocation.PageIndex << 16) | blockIndex
  → 推入 FreeList
  → page.AllocatedBlocks--
  → Unlock bucket
```

### 5.3 BigBlock 分配

```
Allocate(size)
  → Align256(size)
  → Lock
  → 在 FreePool[alignedSize] 中查找可复用的 resource
  → if 找到:
      → 取出并返回
  → else:
      → CreateCommittedResource(D3D12_HEAP_TYPE_UPLOAD, alignedSize)
      → Map(0, nullptr, &cpuAddress)
      → 返回 allocation
  → Unlock
```

### 5.4 BigBlock 释放

```
Free(allocation)
  → Lock
  → 将 resource 放入 FreePool[allocation.AllocatedSize]
  → Unlock
```

## 6. 垃圾回收 (GarbageCollect)

每帧调用 `GarbageCollect()`：

### SmallBlock 回收
- 遍历每个 bucket 的 pages
- 如果某个 page 的 `AllocatedBlocks == 0`（全部释放），且该 bucket 有多余的空闲 page
- 标记为可回收，延迟 N 帧后真正释放（Unmap + Release resource）

### BigBlock 回收
- 遍历 FreePool 中的 entry
- 为每个 entry 维护未使用帧计数
- 超过阈值（如 60 帧）未被复用的 resource 真正释放

## 7. 线程安全

- **SmallBlockAllocator**：使用 **per-bucket SpinLock**，不同桶之间无锁竞争。由于 UB 分配通常很快（从 free list 弹出），SpinLock 足够
- **BigBlockAllocator**：使用单一 SpinLock，因为大块分配频率低
- **GarbageCollect**：在 render thread 调用，需要获取所有 bucket lock（顺序获取避免死锁）

## 8. 与现有系统的集成

### 8.1 D3D12RHIConstantBuffer 改造

```cpp
class D3D12RHIConstantBuffer : public RHIConstantBuffer
{
public:
    // 新构造函数：使用 sub-allocation
    D3D12RHIConstantBuffer(RHIResourceDescriptor const& desc, const D3D12ResourceLocation& allocation);

    void* GetResource() const override;  // 返回 allocation.Resource
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
    void* GetMappedAddress() const;      // 返回 allocation.CPUAddress（直接写入，无需 Map/Unmap）

private:
    D3D12ResourceLocation Allocation;    // 替代 ComPtr<ID3D12Resource>
};
```

### 8.2 CBV 创建

CBV 的 `BufferLocation` 直接使用 `Allocation.GPUVirtualAddress`，`SizeInBytes` 使用 `Allocation.AllocatedSize`。无需改变 offline descriptor 的分配逻辑。

### 8.3 数据写入

由于 upload heap page 是常驻映射的（创建时 Map，销毁时 Unmap），写入数据直接通过 `Allocation.CPUAddress` 用 `memcpy` 即可，不需要每次 Map/Unmap。

### 8.4 释放流程

`RHIUniformBuffer::Update()` 中的 deferred delete 逻辑不变——当 `D3D12RHIConstantBuffer` 引用计数归零时，在析构函数中调用 `PersistentAllocator->Free(Allocation)` 归还 block。

## 9. 关键常量（可配置宏）

```cpp
#define UPLOAD_HEAP_SMALL_BLOCK_THRESHOLD   (64 * 1024)     // 64KB: SmallBlock/BigBlock 分界线
#define UPLOAD_HEAP_PAGE_SIZE               (2 * 1024 * 1024) // 2MB: SmallBlock page 大小
#define UPLOAD_HEAP_GC_EMPTY_PAGE_FRAMES    120              // 空 page 保留帧数
#define UPLOAD_HEAP_GC_BIG_BLOCK_FRAMES     60               // BigBlock 空闲保留帧数
```

## 10. 文件规划

| 文件 | 位置 | 内容 |
|:---|:---|:---|
| `D3D12UploadHeapAllocator.h` | `D3D12RHI/Private/` | 所有数据结构定义 + 类声明 |
| `D3D12UploadHeapAllocator.cpp` | `D3D12RHI/Private/` | 实现 |

Allocator 作为 `D3D12DynamicRHI` 的成员持有，在 RHI 初始化时创建。

## 11. 后续扩展

- **Single Frame Allocator**（Transient）：用于每帧重建的 UB（如 view/projection 矩阵），使用环形 buffer + per-frame 线性分配，帧结束整体 reset，无需 free list。这是下一步要实现的。
- **统计与调试**：添加分配计数、内存使用量、碎片率等统计信息。
