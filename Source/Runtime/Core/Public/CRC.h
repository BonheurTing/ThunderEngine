
#pragma once
#include "Platform.h"
#include "BasicDefinition.h"

namespace Thunder
{
	struct CORE_API FCrc
	{
		/** lookup table with precalculated CRC values - slicing by 8 implementation */
		static uint32 CRCTablesSB8[8][256];
		/** String CRC. */
		template <typename CharType>
		static typename TEnableIf<sizeof(CharType) != 1, uint32>::Type StrCrc32(const CharType* data, uint32 crc = 0)
		{
			// We ensure that we never try to do a StrCrc32 with a CharType of more than 4 bytes.  This is because
			// we always want to treat every CRC as if it was based on 4 byte chars, even if it's less, because we
			// want consistency between equivalent strings with different character types.
			static_assert(sizeof(CharType) <= 4, "StrCrc32 only works with CharType up to 32 bits.");

			crc = ~crc;
			while (CharType Ch = *data++)
			{
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
				Ch >>= 8;
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
				Ch >>= 8;
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
				Ch >>= 8;
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
			}
			return ~crc;
		}

		template <typename CharType>
		static typename TEnableIf<sizeof(CharType) == 1, uint32>::Type StrCrc32(const CharType* data, uint32 crc = 0)
		{
			/* Overload for when CharType is a byte, which causes warnings when right-shifting by 8 */
			crc = ~crc;
			while (CharType Ch = *data++)
			{
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc ^ Ch) & 0xFF];
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc     ) & 0xFF];
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc     ) & 0xFF];
				crc = (crc >> 8) ^ CRCTablesSB8[0][(crc     ) & 0xFF];
			}
			return ~crc;
		}

		/** Binary data CRC. */
		static uint32 BinaryCrc32(const uint8* byteData, uint32 size, uint32 crc = 0);
	};
}
