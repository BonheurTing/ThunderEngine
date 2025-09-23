#pragma once
#include "BasicDefinition.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    
    /* Unsafe Binary Data*/
    struct BinaryData : RefCountedObject
    {
        void* Data = nullptr;
        size_t Size = 0;
    };
    using BinaryDataRef = TRefCountPtr<BinaryData>;

    /* Safe Binary Data*/
    struct ManagedBinaryData
    {
        ManagedBinaryData() = default;
        ManagedBinaryData(const void* inData, size_t inSize)
            : Size(inSize)
        {
            Data = TMemory::Malloc<uint8>(inSize);
            memcpy(Data, inData, inSize);
        }
        ~ManagedBinaryData()
        {
            TMemory::Destroy(Data);
        }
        void SetData(const void* inData, size_t inSize)
        {
            if (Data)
            {
                TMemory::Destroy(Data);
            }
            Size = inSize;
            Data = TMemory::Malloc<uint8>(inSize);
            memcpy(Data, inData, inSize);
        }
        _NODISCARD_ void* GetData()
        {
            return Data;
        }
        _NODISCARD_ size_t GetSize()
        {
            return Size;
        }
    protected:
        void* Data = nullptr;
        size_t Size = 0;
    };
}
