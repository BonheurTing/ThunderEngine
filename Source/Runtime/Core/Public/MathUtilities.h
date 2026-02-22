#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "FileSystem/MemoryArchive.h"
#include <cmath>

#define DEG_TO_RAD 0.0174532925f

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

        /**
         * Build a perspective projection matrix matching Unreal Engine's coordinate conventions.
         *
         * Coordinate system: left-handed, Z-up (X forward, Y right, Z up).
         * Depth range: Reverse-Z, NDC Z in [0, 1] where near plane maps to 1 and far plane maps to 0.
         * Transform convention: column vector, row-major storage (v' = M * v, same as UE FPerspectiveMatrix).
         *
         * When bInfiniteFar is true the far plane is pushed to infinity, which eliminates far-plane
         * clipping artefacts and is the default used by UE's deferred renderer.  In that case the
         * InFar parameter is ignored.
         *
         * @param InFovY         Vertical full field-of-view in radians.
         * @param InAspect       Viewport aspect ratio: width / height.
         * @param InNear         Distance to the near clip plane (must be > 0).
         * @param InFar          Distance to the far clip plane (ignored when bInfiniteFar is true).
         * @param bInfiniteFar   When true, produce an infinite-far reverse-Z projection.
         * @return               Row-major 4x4 perspective projection matrix.
         */
        FORCEINLINE TMatrix44f PerspectiveProjectionMatrix(
            float InFovY,
            float InAspect,
            float InNear,
            float InFar,
            bool  bInfiniteFar = false)
        {
            const float halfFov = InFovY * 0.5f;
            const float cotY    = std::cos(halfFov) / std::sin(halfFov);  // cot(halfFov)
            const float cotX    = cotY / InAspect;

            // Reverse-Z: near -> 1, far -> 0.
            // Column-vector convention, row-major storage.
            //
            // Finite far:
            //   M[2][2] = -InNear / (InFar - InNear)          (== near/(near-far) since reverse-Z)
            //   M[2][3] =  InNear * InFar / (InFar - InNear)
            //
            // Infinite far (InFar -> infinity):
            //   M[2][2] = 0
            //   M[2][3] = InNear

            TMatrix44f result(0.f);
            result.M[0][0] = cotX;
            result.M[1][1] = cotY;

            if (bInfiniteFar)
            {
                result.M[2][2] = 0.f;
                result.M[2][3] = InNear;
            }
            else
            {
                const float invRange = 1.f / (InFar - InNear);
                result.M[2][2] = -InNear * invRange;           // near / (near - far)
                result.M[2][3] =  InNear * InFar * invRange;   // near*far / (far - near)
            }

            result.M[3][2] = 1.f;  // pass-through w = z (left-handed, z forward)

            return result;
        }

    }
}
