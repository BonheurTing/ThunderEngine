#pragma once

#include "Vector.h"
#include "FileSystem/MemoryArchive.h"


namespace Thunder
{
    struct AABB
    {
        TVector3f Min { TVector3f(0.f, 0.f, 0.f) };
        TVector3f Max { TVector3f(0.f, 0.f, 0.f) };

        AABB() = default;
        AABB(const TVector3f& min, const TVector3f& max)
            : Min(min), Max(max) {}
        ~AABB() = default;

        void Serialize(MemoryWriter& archive) const
        {
            archive << Min;
            archive << Max;
        }
        void DeSerialize(MemoryReader& archive)
        {
            archive >> Min;
            archive >> Max;
        }
    };
}
