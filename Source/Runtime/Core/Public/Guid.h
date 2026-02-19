#pragma once
#include <format>
#include "Assertion.h"

namespace Thunder
{
	struct CORE_API TGuid
	{
		uint32 A, B, C, D;

		TGuid() : A(0), B(0), C(0), D(0) {}

		TGuid(uint32 InA, uint32 InB, uint32 InC, uint32 InD)
			: A(InA), B(InB), C(InC), D(InD)
		{
			TAssertf(!IsSystemValue(), "Invalid guid.");
		}

		// System value.
		TGuid(uint32 InD)
			: A(0xFFFFFFFF), B(0xFFFFFFFF), C(0xFFFFFFFF), D(InD) {}

		bool IsValid() const
		{
			return (A | B | C | D) != 0;
		}
		
		bool operator==(const TGuid& X) const
		{
			return A == X.A && B == X.B && C == X.C && D == X.D;
		}

		bool operator<(const TGuid& other) const {
			if (A == other.A) {
				if (B == other.B)
				{
					if (C == other.C)
					{
						return D < other.D;
					}
					return C < other.C;
				}
				return B < other.B;
			}
			return A < other.A;
		}

		String ToString() const
		{
			return std::format("{:08X}-{:08X}-{:08X}-{:08X}", A, B, C, D);
		}

		static void GenerateGUID(TGuid& Result)
		{
			TAssert(CoCreateGuid(reinterpret_cast<GUID*>(&Result)) == S_OK);
			while (Result.IsSystemValue())
			{
				TAssert(CoCreateGuid(reinterpret_cast<GUID*>(&Result)) == S_OK);
			}
		}

		bool IsSystemValue() const
		{
			return A == 0xFFFFFFFF && B == 0xFFFFFFFF && C == 0xFFFFFFFF;
		}
	};
}

// std::hash specialization for TGuid
namespace std
{
	template<>
	struct hash<Thunder::TGuid>
	{
		size_t operator()(const Thunder::TGuid& guid) const noexcept
		{
			// Combine the four 32-bit components into a hash
			size_t hash = guid.A;
			hash ^= guid.B + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= guid.C + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= guid.D + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			return hash;
		}
	};
}
