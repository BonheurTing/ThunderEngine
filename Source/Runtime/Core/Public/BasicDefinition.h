#pragma once
#pragma warning(error:4834)

#include "Platform.h"
#include "Container.h"

#define _NODISCARD_ [[nodiscard]]
#define THUNDER_ENGINE_VERSION 123
#define WITH_EDITOR 1

namespace Thunder
{
	template <bool Predicate, typename Result = void>
	class TEnableIf;

	template <typename Result>
	class TEnableIf<true, Result>
	{
	public:
		using type = Result;
		using Type = Result;
	};

	template <typename Result>
	class TEnableIf<false, Result>
	{ };

	// root signature
	struct TShaderRegisterCounts
	{
		union
		{
			uint8 Hash[4] { 0, 0, 0, 0 };
			struct
			{
				uint8 SamplerCount;
				uint8 ConstantBufferCount;
				uint8 ShaderResourceCount;
				uint8 UnorderedAccessCount;
			};
		};
		inline bool operator==(const TShaderRegisterCounts& rhs) const
		{
			return 0 == memcmp(this, &rhs, sizeof(rhs));
		}
	};

	template<typename Enum>
	constexpr bool EnumHasAnyFlags(Enum flags, Enum contains)
	{
		return ( static_cast<__underlying_type(Enum)>(flags) & static_cast<__underlying_type(Enum)>(contains) ) != 0;
	}
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
