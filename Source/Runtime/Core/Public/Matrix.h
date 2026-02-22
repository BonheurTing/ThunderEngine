#pragma once

#include "Platform.h"
#include "Vector.h"
#include <cmath>
#include <cstring>

namespace Thunder
{
    /**
     * 4x4 matrix of floating point values.
     * Matrix elements are accessed with M[RowIndex][ColumnIndex].
     */
    template<typename T>
    struct alignas(16) TMatrix
    {
        static_assert(std::is_floating_point_v<T>, "T must be floating point");

        union
        {
            T M[4][4];
            T Raw[16];
        };

        /** Default constructor, initializes to identity. */
        FORCEINLINE TMatrix();

        /**
         * Constructor initializing all elements to zero.
         *
         * @param InF Value to set all elements to (typically 0).
         */
        explicit FORCEINLINE TMatrix(T InF);

        /**
         * Constructor from four row vectors (TVector4).
         *
         * @param InX First row.
         * @param InY Second row.
         * @param InZ Third row.
         * @param InW Fourth row.
         */
        FORCEINLINE TMatrix(const TVector4<T>& InX, const TVector4<T>& InY, const TVector4<T>& InZ, const TVector4<T>& InW);

        /**
         * Constructor from four 3D row vectors.
         * The fourth column is set to (0, 0, 0, 1) for a standard affine matrix.
         *
         * @param InX First row (xyz).
         * @param InY Second row (xyz).
         * @param InZ Third row (xyz).
         * @param InW Fourth row (xyz), treated as translation.
         */
        FORCEINLINE TMatrix(const TVector<T>& InX, const TVector<T>& InY, const TVector<T>& InZ, const TVector<T>& InW);

        /**
         * Constructor from 16 individual values (row-major order).
         */
        FORCEINLINE TMatrix(
            T M00, T M01, T M02, T M03,
            T M10, T M11, T M12, T M13,
            T M20, T M21, T M22, T M23,
            T M30, T M31, T M32, T M33);

        /**
         * Conversion constructor from a matrix of a different floating point type.
         */
        template<typename U>
        explicit FORCEINLINE TMatrix(const TMatrix<U>& From);

        /** Set this to the identity matrix. */
        FORCEINLINE void SetIdentity();

        // ----- Operators -----

        /** Matrix multiplication. */
        FORCEINLINE TMatrix<T> operator*(const TMatrix<T>& Other) const;

        /** Matrix multiplication assignment. */
        FORCEINLINE void operator*=(const TMatrix<T>& Other);

        /** Matrix addition. */
        FORCEINLINE TMatrix<T> operator+(const TMatrix<T>& Other) const;

        /** Matrix addition assignment. */
        FORCEINLINE void operator+=(const TMatrix<T>& Other);

        /** Scalar multiplication (scales all elements). */
        FORCEINLINE TMatrix<T> operator*(T Scalar) const;

        /** Scalar multiplication assignment. */
        FORCEINLINE void operator*=(T Scalar);

        /** Equality comparison. */
        inline bool operator==(const TMatrix<T>& Other) const;

        /** Inequality comparison. */
        inline bool operator!=(const TMatrix<T>& Other) const;

        // ----- Transform -----

        /** Transform a 4D vector by this matrix. */
        FORCEINLINE TVector4<T> TransformVector4(const TVector4<T>& V) const;

        /** Transform a position (w=1), accounting for translation. */
        FORCEINLINE TVector4<T> TransformPosition(const TVector<T>& V) const;

        /** Transform a direction (w=0), ignoring translation. */
        FORCEINLINE TVector4<T> TransformDirection(const TVector<T>& V) const;

        // ----- Matrix operations -----

        /** @return The transposed matrix. */
        FORCEINLINE TMatrix<T> GetTransposed() const;

        /** @return The determinant of this matrix. */
        inline T Determinant() const;

        /** @return The determinant of the upper-left 3x3 rotation part. */
        inline T RotDeterminant() const;

        /** @return The inverse of this matrix. Returns identity if non-invertible. */
        inline TMatrix<T> Inverse() const;

        /** @return The transpose of the adjoint (cofactor) 3x3 part. */
        inline TMatrix<T> TransposeAdjoint() const;

        // ----- Translation -----

        /** @return The translation component (row 3, xyz). */
        FORCEINLINE TVector<T> GetOrigin() const;

        /** Set the translation component. */
        FORCEINLINE void SetOrigin(const TVector<T>& NewOrigin);

        /** @return A copy with translation removed. */
        inline TMatrix<T> RemoveTranslation() const;

        // ----- Axis access -----

        /** Get a row axis (0=X, 1=Y, 2=Z) as a 3D vector. */
        FORCEINLINE TVector<T> GetScaledAxis(int32 Axis) const;

        /** Get a column as a 3D vector (from rows 0-2). */
        FORCEINLINE TVector4<T> GetColumn(int32 Col) const;

        /** Get a row as a vector4. */
        FORCEINLINE TVector4<T> GetRow(int32 Row) const;

        /** Set a row axis (0=X, 1=Y, 2=Z). */
        FORCEINLINE void SetAxis(int32 Axis, const TVector<T>& Value);

        /** Set a column (rows 0-2). */
        FORCEINLINE void SetColumn(int32 Col, const TVector<T>& Value);

        // ----- Scale -----

        /** @return 3D scale vector (magnitude of each row). */
        inline TVector<T> GetScaleVector() const;

        /** Apply uniform scale. */
        inline TMatrix<T> ApplyScale(T Scale) const;

        // ----- Utility -----

        /** @return true if any element is NaN or infinite. */
        inline bool ContainsNaN() const;

        /**
         * Convert to 3x4 transposed layout (column-major 3 rows, 4 cols).
         *
         * @param Out Output buffer, must have at least 12 elements.
         */
        FORCEINLINE void To3x4MatrixTranspose(T* Out) const;

        // ----- Static helpers -----

        /** @return The identity matrix. */
        static FORCEINLINE TMatrix<T> Identity();
    };

    // =========================================================================
    // Implementation
    // =========================================================================

    template<typename T>
    FORCEINLINE TMatrix<T>::TMatrix()
    {
        M[0][0] = 1; M[0][1] = 0; M[0][2] = 0; M[0][3] = 0;
        M[1][0] = 0; M[1][1] = 1; M[1][2] = 0; M[1][3] = 0;
        M[2][0] = 0; M[2][1] = 0; M[2][2] = 1; M[2][3] = 0;
        M[3][0] = 0; M[3][1] = 0; M[3][2] = 0; M[3][3] = 1;
    }

    template<typename T>
    FORCEINLINE TMatrix<T>::TMatrix(T InF)
    {
        for (int32 i = 0; i < 16; i++)
        {
            Raw[i] = InF;
        }
    }

    template<typename T>
    FORCEINLINE TMatrix<T>::TMatrix(const TVector4<T>& InX, const TVector4<T>& InY, const TVector4<T>& InZ, const TVector4<T>& InW)
    {
        M[0][0] = InX.X; M[0][1] = InX.Y; M[0][2] = InX.Z; M[0][3] = InX.W;
        M[1][0] = InY.X; M[1][1] = InY.Y; M[1][2] = InY.Z; M[1][3] = InY.W;
        M[2][0] = InZ.X; M[2][1] = InZ.Y; M[2][2] = InZ.Z; M[2][3] = InZ.W;
        M[3][0] = InW.X; M[3][1] = InW.Y; M[3][2] = InW.Z; M[3][3] = InW.W;
    }

    template<typename T>
    FORCEINLINE TMatrix<T>::TMatrix(const TVector<T>& InX, const TVector<T>& InY, const TVector<T>& InZ, const TVector<T>& InW)
    {
        M[0][0] = InX.X; M[0][1] = InX.Y; M[0][2] = InX.Z; M[0][3] = 0;
        M[1][0] = InY.X; M[1][1] = InY.Y; M[1][2] = InY.Z; M[1][3] = 0;
        M[2][0] = InZ.X; M[2][1] = InZ.Y; M[2][2] = InZ.Z; M[2][3] = 0;
        M[3][0] = InW.X; M[3][1] = InW.Y; M[3][2] = InW.Z; M[3][3] = 1;
    }

    template<typename T>
    FORCEINLINE TMatrix<T>::TMatrix(
        T M00, T M01, T M02, T M03,
        T M10, T M11, T M12, T M13,
        T M20, T M21, T M22, T M23,
        T M30, T M31, T M32, T M33)
    {
        M[0][0] = M00; M[0][1] = M01; M[0][2] = M02; M[0][3] = M03;
        M[1][0] = M10; M[1][1] = M11; M[1][2] = M12; M[1][3] = M13;
        M[2][0] = M20; M[2][1] = M21; M[2][2] = M22; M[2][3] = M23;
        M[3][0] = M30; M[3][1] = M31; M[3][2] = M32; M[3][3] = M33;
    }

    template<typename T>
    template<typename U>
    FORCEINLINE TMatrix<T>::TMatrix(const TMatrix<U>& From)
    {
        M[0][0] = (T)From.M[0][0]; M[0][1] = (T)From.M[0][1]; M[0][2] = (T)From.M[0][2]; M[0][3] = (T)From.M[0][3];
        M[1][0] = (T)From.M[1][0]; M[1][1] = (T)From.M[1][1]; M[1][2] = (T)From.M[1][2]; M[1][3] = (T)From.M[1][3];
        M[2][0] = (T)From.M[2][0]; M[2][1] = (T)From.M[2][1]; M[2][2] = (T)From.M[2][2]; M[2][3] = (T)From.M[2][3];
        M[3][0] = (T)From.M[3][0]; M[3][1] = (T)From.M[3][1]; M[3][2] = (T)From.M[3][2]; M[3][3] = (T)From.M[3][3];
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::SetIdentity()
    {
        M[0][0] = 1; M[0][1] = 0; M[0][2] = 0; M[0][3] = 0;
        M[1][0] = 0; M[1][1] = 1; M[1][2] = 0; M[1][3] = 0;
        M[2][0] = 0; M[2][1] = 0; M[2][2] = 1; M[2][3] = 0;
        M[3][0] = 0; M[3][1] = 0; M[3][2] = 0; M[3][3] = 1;
    }

    // ----- Operators -----

    template<typename T>
    FORCEINLINE TMatrix<T> TMatrix<T>::operator*(const TMatrix<T>& Other) const
    {
        TMatrix<T> Result;
        for (int32 row = 0; row < 4; row++)
        {
            for (int32 col = 0; col < 4; col++)
            {
                Result.M[row][col] =
                    M[row][0] * Other.M[0][col] +
                    M[row][1] * Other.M[1][col] +
                    M[row][2] * Other.M[2][col] +
                    M[row][3] * Other.M[3][col];
            }
        }
        return Result;
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::operator*=(const TMatrix<T>& Other)
    {
        *this = *this * Other;
    }

    template<typename T>
    FORCEINLINE TMatrix<T> TMatrix<T>::operator+(const TMatrix<T>& Other) const
    {
        TMatrix<T> Result(T(0));
        for (int32 i = 0; i < 4; i++)
        {
            Result.M[i][0] = M[i][0] + Other.M[i][0];
            Result.M[i][1] = M[i][1] + Other.M[i][1];
            Result.M[i][2] = M[i][2] + Other.M[i][2];
            Result.M[i][3] = M[i][3] + Other.M[i][3];
        }
        return Result;
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::operator+=(const TMatrix<T>& Other)
    {
        *this = *this + Other;
    }

    template<typename T>
    FORCEINLINE TMatrix<T> TMatrix<T>::operator*(T Scalar) const
    {
        TMatrix<T> Result(T(0));
        for (int32 i = 0; i < 4; i++)
        {
            Result.M[i][0] = M[i][0] * Scalar;
            Result.M[i][1] = M[i][1] * Scalar;
            Result.M[i][2] = M[i][2] * Scalar;
            Result.M[i][3] = M[i][3] * Scalar;
        }
        return Result;
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::operator*=(T Scalar)
    {
        *this = *this * Scalar;
    }

    template<typename T>
    inline bool TMatrix<T>::operator==(const TMatrix<T>& Other) const
    {
        for (int32 i = 0; i < 4; i++)
        {
            for (int32 j = 0; j < 4; j++)
            {
                if (M[i][j] != Other.M[i][j])
                {
                    return false;
                }
            }
        }
        return true;
    }

    template<typename T>
    inline bool TMatrix<T>::operator!=(const TMatrix<T>& Other) const
    {
        return !(*this == Other);
    }

    // ----- Transform -----

    template<typename T>
    FORCEINLINE TVector4<T> TMatrix<T>::TransformVector4(const TVector4<T>& V) const
    {
        return TVector4<T>(
            V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0] + V.W * M[3][0],
            V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1] + V.W * M[3][1],
            V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2] + V.W * M[3][2],
            V.X * M[0][3] + V.Y * M[1][3] + V.Z * M[2][3] + V.W * M[3][3]);
    }

    template<typename T>
    FORCEINLINE TVector4<T> TMatrix<T>::TransformPosition(const TVector<T>& V) const
    {
        return TransformVector4(TVector4<T>(V.X, V.Y, V.Z, T(1)));
    }

    template<typename T>
    FORCEINLINE TVector4<T> TMatrix<T>::TransformDirection(const TVector<T>& V) const
    {
        return TransformVector4(TVector4<T>(V.X, V.Y, V.Z, T(0)));
    }

    // ----- Matrix operations -----

    template<typename T>
    FORCEINLINE TMatrix<T> TMatrix<T>::GetTransposed() const
    {
        TMatrix<T> Result(T(0));
        Result.M[0][0] = M[0][0]; Result.M[0][1] = M[1][0]; Result.M[0][2] = M[2][0]; Result.M[0][3] = M[3][0];
        Result.M[1][0] = M[0][1]; Result.M[1][1] = M[1][1]; Result.M[1][2] = M[2][1]; Result.M[1][3] = M[3][1];
        Result.M[2][0] = M[0][2]; Result.M[2][1] = M[1][2]; Result.M[2][2] = M[2][2]; Result.M[2][3] = M[3][2];
        Result.M[3][0] = M[0][3]; Result.M[3][1] = M[1][3]; Result.M[3][2] = M[2][3]; Result.M[3][3] = M[3][3];
        return Result;
    }

    template<typename T>
    inline T TMatrix<T>::Determinant() const
    {
        return
            M[0][0] * (
                M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) +
                M[3][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2])
            ) -
            M[1][0] * (
                M[0][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                M[2][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
                M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2])
            ) +
            M[2][0] * (
                M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) -
                M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
                M[3][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
            ) -
            M[3][0] * (
                M[0][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) -
                M[1][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) +
                M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
            );
    }

    template<typename T>
    inline T TMatrix<T>::RotDeterminant() const
    {
        return
            M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
            M[1][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1]) +
            M[2][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1]);
    }

    template<typename T>
    inline TMatrix<T> TMatrix<T>::Inverse() const
    {
        const T* src = &M[0][0];

        T inv[16];
        inv[0]  =  src[5] * src[10] * src[15] - src[5] * src[11] * src[14] - src[9] * src[6] * src[15] + src[9] * src[7] * src[14] + src[13] * src[6] * src[11] - src[13] * src[7] * src[10];
        inv[1]  = -src[1] * src[10] * src[15] + src[1] * src[11] * src[14] + src[9] * src[2] * src[15] - src[9] * src[3] * src[14] - src[13] * src[2] * src[11] + src[13] * src[3] * src[10];
        inv[2]  =  src[1] * src[6]  * src[15] - src[1] * src[7]  * src[14] - src[5] * src[2] * src[15] + src[5] * src[3] * src[14] + src[13] * src[2] * src[7]  - src[13] * src[3] * src[6];
        inv[3]  = -src[1] * src[6]  * src[11] + src[1] * src[7]  * src[10] + src[5] * src[2] * src[11] - src[5] * src[3] * src[10] - src[9]  * src[2] * src[7]  + src[9]  * src[3] * src[6];
        inv[4]  = -src[4] * src[10] * src[15] + src[4] * src[11] * src[14] + src[8] * src[6] * src[15] - src[8] * src[7] * src[14] - src[12] * src[6] * src[11] + src[12] * src[7] * src[10];
        inv[5]  =  src[0] * src[10] * src[15] - src[0] * src[11] * src[14] - src[8] * src[2] * src[15] + src[8] * src[3] * src[14] + src[12] * src[2] * src[11] - src[12] * src[3] * src[10];
        inv[6]  = -src[0] * src[6]  * src[15] + src[0] * src[7]  * src[14] + src[4] * src[2] * src[15] - src[4] * src[3] * src[14] - src[12] * src[2] * src[7]  + src[12] * src[3] * src[6];
        inv[7]  =  src[0] * src[6]  * src[11] - src[0] * src[7]  * src[10] - src[4] * src[2] * src[11] + src[4] * src[3] * src[10] + src[8]  * src[2] * src[7]  - src[8]  * src[3] * src[6];
        inv[8]  =  src[4] * src[9]  * src[15] - src[4] * src[11] * src[13] - src[8] * src[5] * src[15] + src[8] * src[7] * src[13] + src[12] * src[5] * src[11] - src[12] * src[7] * src[9];
        inv[9]  = -src[0] * src[9]  * src[15] + src[0] * src[11] * src[13] + src[8] * src[1] * src[15] - src[8] * src[3] * src[13] - src[12] * src[1] * src[11] + src[12] * src[3] * src[9];
        inv[10] =  src[0] * src[5]  * src[15] - src[0] * src[7]  * src[13] - src[4] * src[1] * src[15] + src[4] * src[3] * src[13] + src[12] * src[1] * src[7]  - src[12] * src[3] * src[5];
        inv[11] = -src[0] * src[5]  * src[11] + src[0] * src[7]  * src[9]  + src[4] * src[1] * src[11] - src[4] * src[3] * src[9]  - src[8]  * src[1] * src[7]  + src[8]  * src[3] * src[5];
        inv[12] = -src[4] * src[9]  * src[14] + src[4] * src[10] * src[13] + src[8] * src[5] * src[14] - src[8] * src[6] * src[13] - src[12] * src[5] * src[10] + src[12] * src[6] * src[9];
        inv[13] =  src[0] * src[9]  * src[14] - src[0] * src[10] * src[13] - src[8] * src[1] * src[14] + src[8] * src[2] * src[13] + src[12] * src[1] * src[10] - src[12] * src[2] * src[9];
        inv[14] = -src[0] * src[5]  * src[14] + src[0] * src[6]  * src[13] + src[4] * src[1] * src[14] - src[4] * src[2] * src[13] - src[12] * src[1] * src[6]  + src[12] * src[2] * src[5];
        inv[15] =  src[0] * src[5]  * src[10] - src[0] * src[6]  * src[9]  - src[4] * src[1] * src[10] + src[4] * src[2] * src[9]  + src[8]  * src[1] * src[6]  - src[8]  * src[2] * src[5];

        T det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];
        if (det == T(0))
        {
            return Identity();
        }

        TMatrix<T> Result(T(0));
        det = T(1) / det;
        for (int32 i = 0; i < 16; i++)
        {
            Result.Raw[i] = inv[i] * det;
        }
        return Result;
    }

    template<typename T>
    inline TMatrix<T> TMatrix<T>::TransposeAdjoint() const
    {
        TMatrix<T> TA(T(0));

        TA.M[0][0] = M[1][1] * M[2][2] - M[1][2] * M[2][1];
        TA.M[0][1] = M[1][2] * M[2][0] - M[1][0] * M[2][2];
        TA.M[0][2] = M[1][0] * M[2][1] - M[1][1] * M[2][0];
        TA.M[0][3] = 0;

        TA.M[1][0] = M[2][1] * M[0][2] - M[2][2] * M[0][1];
        TA.M[1][1] = M[2][2] * M[0][0] - M[2][0] * M[0][2];
        TA.M[1][2] = M[2][0] * M[0][1] - M[2][1] * M[0][0];
        TA.M[1][3] = 0;

        TA.M[2][0] = M[0][1] * M[1][2] - M[0][2] * M[1][1];
        TA.M[2][1] = M[0][2] * M[1][0] - M[0][0] * M[1][2];
        TA.M[2][2] = M[0][0] * M[1][1] - M[0][1] * M[1][0];
        TA.M[2][3] = 0;

        TA.M[3][0] = 0;
        TA.M[3][1] = 0;
        TA.M[3][2] = 0;
        TA.M[3][3] = 1;

        return TA;
    }

    // ----- Translation -----

    template<typename T>
    FORCEINLINE TVector<T> TMatrix<T>::GetOrigin() const
    {
        return TVector<T>(M[3][0], M[3][1], M[3][2]);
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::SetOrigin(const TVector<T>& NewOrigin)
    {
        M[3][0] = NewOrigin.X;
        M[3][1] = NewOrigin.Y;
        M[3][2] = NewOrigin.Z;
    }

    template<typename T>
    inline TMatrix<T> TMatrix<T>::RemoveTranslation() const
    {
        TMatrix<T> Result = *this;
        Result.M[3][0] = T(0);
        Result.M[3][1] = T(0);
        Result.M[3][2] = T(0);
        return Result;
    }

    // ----- Axis access -----

    template<typename T>
    FORCEINLINE TVector<T> TMatrix<T>::GetScaledAxis(int32 Axis) const
    {
        return TVector<T>(M[Axis][0], M[Axis][1], M[Axis][2]);
    }

    template<typename T>
    FORCEINLINE TVector4<T> TMatrix<T>::GetColumn(int32 Col) const
    {
        return TVector4<T>(M[0][Col], M[1][Col], M[2][Col], M[3][Col]);
    }

    template <typename T>
    TVector4<T> TMatrix<T>::GetRow(int32 Row) const
    {
        return TVector4<T>(M[Row][0], M[Row][1], M[Row][2], M[Row][3]);
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::SetAxis(int32 Axis, const TVector<T>& Value)
    {
        M[Axis][0] = Value.X;
        M[Axis][1] = Value.Y;
        M[Axis][2] = Value.Z;
        M[Axis][3] = Value.W;
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::SetColumn(int32 Col, const TVector<T>& Value)
    {
        M[0][Col] = Value.X;
        M[1][Col] = Value.Y;
        M[2][Col] = Value.Z;
        M[3][Col] = Value.W;
    }

    // ----- Scale -----

    template<typename T>
    inline TVector<T> TMatrix<T>::GetScaleVector() const
    {
        return TVector<T>(
            std::sqrt(M[0][0] * M[0][0] + M[0][1] * M[0][1] + M[0][2] * M[0][2]),
            std::sqrt(M[1][0] * M[1][0] + M[1][1] * M[1][1] + M[1][2] * M[1][2]),
            std::sqrt(M[2][0] * M[2][0] + M[2][1] * M[2][1] + M[2][2] * M[2][2]));
    }

    template<typename T>
    inline TMatrix<T> TMatrix<T>::ApplyScale(T Scale) const
    {
        TMatrix<T> ScaleMatrix;
        ScaleMatrix.M[0][0] = Scale; ScaleMatrix.M[0][1] = 0;     ScaleMatrix.M[0][2] = 0;     ScaleMatrix.M[0][3] = 0;
        ScaleMatrix.M[1][0] = 0;     ScaleMatrix.M[1][1] = Scale; ScaleMatrix.M[1][2] = 0;     ScaleMatrix.M[1][3] = 0;
        ScaleMatrix.M[2][0] = 0;     ScaleMatrix.M[2][1] = 0;     ScaleMatrix.M[2][2] = Scale; ScaleMatrix.M[2][3] = 0;
        ScaleMatrix.M[3][0] = 0;     ScaleMatrix.M[3][1] = 0;     ScaleMatrix.M[3][2] = 0;     ScaleMatrix.M[3][3] = 1;
        return ScaleMatrix * (*this);
    }

    // ----- Utility -----

    template<typename T>
    inline bool TMatrix<T>::ContainsNaN() const
    {
        for (int32 i = 0; i < 16; i++)
        {
            if (!std::isfinite(Raw[i]))
            {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    FORCEINLINE void TMatrix<T>::To3x4MatrixTranspose(T* Out) const
    {
        Out[0]  = M[0][0]; Out[1]  = M[1][0]; Out[2]  = M[2][0]; Out[3]  = M[3][0];
        Out[4]  = M[0][1]; Out[5]  = M[1][1]; Out[6]  = M[2][1]; Out[7]  = M[3][1];
        Out[8]  = M[0][2]; Out[9]  = M[1][2]; Out[10] = M[2][2]; Out[11] = M[3][2];
    }

    template<typename T>
    FORCEINLINE TMatrix<T> TMatrix<T>::Identity()
    {
        return TMatrix<T>();
    }

    // =========================================================================
    // Type aliases
    // =========================================================================

    using TMatrix44f = TMatrix<float>;
    using TMatrix44d = TMatrix<double>;
}
