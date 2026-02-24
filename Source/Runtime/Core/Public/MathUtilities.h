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
         * === Coordinate system ===
         *   Left-handed, Z-up: X=Forward, Y=Right, Z=Up.
         *
         * === Pipeline transform convention ===
         *   CPU matrices are row-major (M[row][col]), translation in row 3.
         *   GPU upload: each matrix is decomposed by GetColumn() and reassembled in HLSL as
         *   float4x4(col0,col1,col2,col3), which transposes the matrix on the way to the GPU.
         *   HLSL shaders therefore use column-vector multiply: p_clip = M_gpu * p_view.
         *   Net result: p_clip = M_cpu^T * p_view  (column-vector, X-forward).
         *
         * === Projection mapping (column-vector, X-forward, reverse-Z) ===
         *   w_clip  = X_view              (X is the forward / depth axis)
         *   NDC_x   = Y_view * cotX / w   (Y=Right  -> screen horizontal)
         *   NDC_y   = Z_view * cotY / w   (Z=Up     -> screen vertical)
         *   NDC_z   = (A*X_view + B) / w  (reverse-Z: near->1, far->0)
         *
         * The GPU-side matrix M_gpu that achieves this is:
         *
         *              col_x  col_y  col_z  col_w
         *   row_0   [   0,    cotX,   0,     0  ]   p_clip.x = cotX * Y_view
         *   row_1   [   0,     0,    cotY,   0  ]   p_clip.y = cotY * Z_view
         *   row_2   [   A,     0,     0,     B  ]   p_clip.z = A*X + B
         *   row_3   [   1,     0,     0,     0  ]   p_clip.w = X_view
         *
         * Transposing back gives the CPU-side layout stored in M[row][col]:
         *
         *              col 0  col 1  col 2  col 3
         *   row 0   [   0,     0,     A,     1  ]
         *   row 1   [  cotX,   0,     0,     0  ]
         *   row 2   [   0,    cotY,   0,     0  ]
         *   row 3   [   0,     0,     B,     0  ]
         *
         * Finite far:
         *   A = InNear / (InNear - InFar)     (positive, reverse-Z)
         *   B = InNear * InFar / (InFar - InNear)
         *
         * Infinite far (InFar -> infinity):
         *   A = 0,  B = InNear
         *
         * When bInfiniteFar is true the far plane is pushed to infinity, which eliminates far-plane
         * clipping artefacts and is the default used by UE's deferred renderer.  InFar is ignored.
         *
         * @param InFovY         Vertical full field-of-view in radians.
         * @param InAspect       Viewport aspect ratio: width / height.
         * @param InNear         Distance to the near clip plane (must be > 0).
         * @param InFar          Distance to the far clip plane (ignored when bInfiniteFar is true).
         * @param bInfiniteFar   When true, produce an infinite-far reverse-Z projection.
         * @return               CPU-side row-major matrix (transposed on GPU upload for column-vector HLSL).
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

            // CPU-side layout. The upload path calls GetColumn() and HLSL reconstructs with
            // float4x4(col0,col1,col2,col3) treating them as rows, which is a transpose.
            // So M_gpu = M_cpu^T, and we must store the transpose of the desired GPU matrix here.
            //
            // Desired GPU matrix (column-vector, X-forward):
            //   row 0: [ 0,    cotX,  0,    0 ]   -> p_clip.x = cotX * Y_view
            //   row 1: [ 0,    0,    cotY,  0 ]   -> p_clip.y = cotY * Z_view
            //   row 2: [ A,    0,    0,     B ]   -> p_clip.z = A*X_view + B
            //   row 3: [ 1,    0,    0,     0 ]   -> p_clip.w = X_view
            //
            // CPU storage M_cpu = M_gpu^T:
            //   M[0][2] = A      M[0][3] = 1
            //   M[1][0] = cotX
            //   M[2][1] = cotY
            //   M[3][2] = B

            TMatrix44f result(0.f);
            result.M[0][3] = 1.f;   // w_clip = X_view  (column of M_gpu^T)
            result.M[1][0] = cotX;  // p_clip.x = cotX * Y_view
            result.M[2][1] = cotY;  // p_clip.y = cotY * Z_view

            if (bInfiniteFar)
            {
                result.M[0][2] = 0.f;
                result.M[3][2] = InNear;
            }
            else
            {
                result.M[0][2] = InNear / (InNear - InFar);             // A
                result.M[3][2] = InNear * InFar / (InFar - InNear);     // B
            }

            return result;
        }

    }
}
