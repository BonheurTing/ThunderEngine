#pragma once

#include <algorithm>

#include "Platform.h"
#include "MathUtilities.h"

namespace Thunder
{
    enum class EMeshPass : uint8
    {
        PrePass = 0,
        BasePass,
        ShadowPass,
        Translucent,
        Num
    };

    class MeshPassMask
    {
    public:
        MeshPassMask()
            : Data(0)
        {
        }

        MeshPassMask(uint64 InData)
            : Data(InData)
        {
        }

        void Set(EMeshPass Pass) 
        { 
            Data |= (static_cast<uint64>(1) << static_cast<uint64>(Pass)); 
        }

        bool Get(EMeshPass Pass) const 
        { 
            return !!(Data & (static_cast<uint64>(1) << static_cast<uint64>(Pass))); 
        }

        EMeshPass SkipEmpty(EMeshPass Pass) const 
        {
            uint64 Mask = 0xFFffFFffFFffFFffULL << static_cast<uint64>(Pass);
            return static_cast<EMeshPass>(std::min<uint64>(static_cast<uint64>(EMeshPass::Num),
                Math::CountTrailingZeros64(Data & Mask)));
        }

        int GetNum() 
        {
            return Math::CountBits(Data); 
        }

        void AppendTo(MeshPassMask& Mask) const 
        { 
            Mask.Data |= Data; 
        }

        void Reset() 
        { 
            Data = 0; 
        }

        bool IsEmpty() const 
        { 
            return Data == 0; 
        }

        uint64 Data;
    };
}

