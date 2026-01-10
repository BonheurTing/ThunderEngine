#pragma once

#include "Vector.h"
#include "FileSystem/MemoryArchive.h"
#include <cmath>


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

    /**
     * Math utility library providing common mathematical operations.
     */
    namespace Math
    {
        /**
         * Generic Distance function template.
         * Calculate the Euclidean distance between two points.
         *
         * @param a First point
         * @param b Second point
         * @return Distance between the two points
         */
        template<typename T>
        T Distance(const T& a, const T& b);

        /**
         * Specialization for TVector2<T> - 2D distance calculation.
         * Distance = sqrt((x2-x1)^2 + (y2-y1)^2)
         *
         * @param a First 2D vector
         * @param b Second 2D vector
         * @return Distance between the two 2D vectors
         */
        template<typename T>
        T Distance(const TVector2<T>& a, const TVector2<T>& b)
        {
            T dx = b.X - a.X;
            T dy = b.Y - a.Y;
            return static_cast<T>(std::sqrt(dx * dx + dy * dy));
        }

        /**
         * Specialization for TVector<T> (3D vector) - 3D distance calculation.
         * Distance = sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2)
         *
         * @param a First 3D vector
         * @param b Second 3D vector
         * @return Distance between the two 3D vectors
         */
        template<typename T>
        T Distance(const TVector<T>& a, const TVector<T>& b)
        {
            T dx = b.X - a.X;
            T dy = b.Y - a.Y;
            T dz = b.Z - a.Z;
            return static_cast<T>(std::sqrt(dx * dx + dy * dy + dz * dz));
        }

        /**
         * Specialization for TVector4<T> - 4D distance calculation.
         * Distance = sqrt((x2-x1)^2 + (y2-y1)^2 + (z2-z1)^2 + (w2-w1)^2)
         *
         * @param a First 4D vector
         * @param b Second 4D vector
         * @return Distance between the two 4D vectors
         */
        template<typename T>
        T Distance(const TVector4<T>& a, const TVector4<T>& b)
        {
            T dx = b.X - a.X;
            T dy = b.Y - a.Y;
            T dz = b.Z - a.Z;
            T dw = b.W - a.W;
            return static_cast<T>(std::sqrt(dx * dx + dy * dy + dz * dz + dw * dw));
        }

        FORCEINLINE uint64 CountTrailingZeros64(uint64 Value)
        {
            unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 63
            return _BitScanForward64( &BitIndex, Value ) ? BitIndex : 64;
        }

        static constexpr FORCEINLINE int32 CountBits(uint64 Bits)
        {
            // https://en.wikipedia.org/wiki/Hamming_weight
            Bits -= (Bits >> 1) & 0x5555555555555555ull;
            Bits = (Bits & 0x3333333333333333ull) + ((Bits >> 2) & 0x3333333333333333ull);
            Bits = (Bits + (Bits >> 4)) & 0x0f0f0f0f0f0f0f0full;
            return (Bits * 0x0101010101010101) >> 56;
        }
    }
}
