#pragma once
#pragma warning(error:4834)

#include "Platform.h"
#include <cstring>
#include "Container.h"

#define _NODISCARD_ [[nodiscard]]

namespace Thunder
{
	/* Unsafe Binary Data*/
	struct BinaryData
	{
		void* Data;
		size_t Size;
	};

	// todo: Safe Binary Data

	// root signature
	struct TShaderRegisterCounts
	{
		uint8 SamplerCount;
		uint8 ConstantBufferCount;
		uint8 ShaderResourceCount;
		uint8 UnorderedAccessCount;

		inline bool operator==(const TShaderRegisterCounts& rhs) const
		{
			return 0 == memcmp(this, &rhs, sizeof(rhs));
		}
	};
}

namespace std
{
	template<>
	struct hash<Thunder::TShaderRegisterCounts>
	{
		size_t operator()(const Thunder::TShaderRegisterCounts& value) const noexcept
		{
			const uint32_t combinedValue = (value.SamplerCount << 24) + (value.ConstantBufferCount << 16) + (value.ShaderResourceCount << 8) + value.UnorderedAccessCount;
			return hash<uint32_t>()(combinedValue);
		}
	};
}
