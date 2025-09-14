#pragma once
#include <format>
#include "Container.h"
#include "Platform.h"

namespace Thunder
{
	struct CORE_API TGuid
	{
		uint32 A, B, C, D;

		TGuid() : A(0), B(0), C(0), D(0) {}

		// 从四个32位整数构造
		TGuid(uint32 InA, uint32 InB, uint32 InC, uint32 InD)
			: A(InA), B(InB), C(InC), D(InD) {}

		TGuid(const TGuid& X) : A(X.A), B(X.B), C(X.C), D(X.D) {}

		bool IsValid() const
		{
			return (A | B | C | D) != 0; // 全0为无效
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
			TAssert( CoCreateGuid( (GUID*)&Result )==S_OK );
			/*Result.A = rand() | (rand() << 16);
			Result.B = rand() | (rand() << 16);
			Result.C = rand() | (rand() << 16);
			Result.D = rand() | (rand() << 16);*/
		}

	};
}
