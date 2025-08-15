#pragma once
#pragma warning(error:4834)

#include "Platform.h"
#include <cstring>
#include "Container.h"
#include "Memory/MemoryBase.h"
#include "Templates/RefCountObject.h"

#define _NODISCARD_ [[nodiscard]]

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
	
	/* Unsafe Binary Data*/
	struct BinaryData : RefCountedObject
	{
		void* Data = nullptr;
		size_t Size = 0;

		
	};

	/* Safe Binary Data*/
	struct ManagedBinaryData
	{
		ManagedBinaryData() = default;
		ManagedBinaryData(const void* inData, size_t inSize)
			: Size(inSize)
		{
			Data = TMemory::Malloc<uint8>(inSize);
			memcpy(Data, inData, inSize);
		}
		~ManagedBinaryData()
		{
			TMemory::Destroy(Data);
		}
		void SetData(const void* inData, size_t inSize)
		{
			if (Data)
			{
				TMemory::Destroy(Data);
			}
			Size = inSize;
			Data = TMemory::Malloc<uint8>(inSize);
			memcpy(Data, inData, inSize);
		}
		_NODISCARD_ void* GetData()
		{
			return Data;
		}
		_NODISCARD_ size_t GetSize()
		{
			return Size;
		}
	protected:
		void* Data = nullptr;
		size_t Size = 0;
	};

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
	constexpr bool EnumHasAnyFlags(Enum Flags, Enum Contains)
	{
		return ( static_cast<__underlying_type(Enum)>(Flags) & static_cast<__underlying_type(Enum)>(Contains) ) != 0;
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
