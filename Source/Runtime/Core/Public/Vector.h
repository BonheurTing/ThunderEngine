#pragma once

namespace Thunder
{
	template<typename T>
	struct TVector
	{
		union
		{
			struct
			{
				/** Vector's X component. */
				T X;

				/** Vector's Y component. */
				T Y;

				/** Vector's Z component. */
				T Z;
			};

			T XYZ[3];
		};

		/** Default constructor (no initialization). */
		FORCEINLINE TVector();

		/**
		 * Constructor initializing all components to a single T value.
		 *
		 * @param InF Value to set all components to.
		 */
		explicit FORCEINLINE TVector(T InF);

		/**
		 * Constructor using initial values for each component.
		 *
		 * @param InX X Coordinate.
		 * @param InY Y Coordinate.
		 * @param InZ Z Coordinate.
		 */
		FORCEINLINE TVector(T InX, T InY, T InZ);
	};

	template<typename T>
	FORCEINLINE TVector<T>::TVector()
		: X(0), Y(0), Z(0)
	{}

	template<typename T>
	FORCEINLINE TVector<T>::TVector(T InF)
		: X(InF), Y(InF), Z(InF)
	{
	}

	template<typename T>
	FORCEINLINE TVector<T>::TVector(T InX, T InY, T InZ)
		: X(InX), Y(InY), Z(InZ)
	{
	}












	
	
	template<typename T>
	struct alignas(16) TVector4
	{
		union
		{
			struct
			{
				/** The vector's X-component. */
				T X;

				/** The vector's Y-component. */
				T Y;

				/** The vector's Z-component. */
				T Z;

				/** The vector's W-component. */
				T W;
			};

			T XYZW[4];
		};

		/**
		 * Constructor from 3D TVector. W is set to 1.
		 *
		 * @param InVector 3D Vector to set first three components.
		 */
		FORCEINLINE TVector4(const TVector<T>& InVector);

		explicit FORCEINLINE TVector4(T InF);
		
		/**
		 * Creates and initializes a new vector from the specified components.
		 *
		 * @param InX X Coordinate.
		 * @param InY Y Coordinate.
		 * @param InZ Z Coordinate.
		 * @param InW W Coordinate.
		 */
		FORCEINLINE TVector4(T InX = 0.0f, T InY = 0.0f, T InZ = 0.0f, T InW = 1.0f);
	};


	template<typename T>
	FORCEINLINE TVector4<T>::TVector4(const TVector<T>& InVector)
		: X(InVector.X)
		, Y(InVector.Y)
		, Z(InVector.Z)
		, W(1.0f)
	{
	}

	template<typename T>
	FORCEINLINE TVector4<T>::TVector4(T InF)
		: X(InF), Y(InF), Z(InF), W(InF)
	{
	}
	
	template<typename T>
	FORCEINLINE TVector4<T>::TVector4(T InX, T InY, T InZ, T InW)
		: X(InX)
		, Y(InY)
		, Z(InZ)
		, W(InW)
	{
	}

	using TVector3f = TVector<float>;
	using TVector4f = TVector4<float>;

	using TVector3d = TVector<double>;
	using TVector4d = TVector4<double>;

	using TVector3i = TVector<int32>;
	using TVector4i = TVector4<int32>;

	using TVector3u = TVector<uint32>;
	using TVector4u = TVector4<uint32>;
	
}
