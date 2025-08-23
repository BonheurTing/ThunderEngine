
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
		static typename TEnableIf<sizeof(CharType) != 1, uint32>::Type StrCrc32(const CharType* Data, uint32 CRC = 0, uint32 DataSize = 0)
		{
			// We ensure that we never try to do a StrCrc32 with a CharType of more than 4 bytes.  This is because
			// we always want to treat every CRC as if it was based on 4 byte chars, even if it's less, because we
			// want consistency between equivalent strings with different character types.
			static_assert(sizeof(CharType) <= 4, "StrCrc32 only works with CharType up to 32 bits.");

			CRC = ~CRC;
			uint32 Counter = DataSize;
			while (CharType Ch = *Data++)
			{
				if(DataSize > 0)
				{
					if (Counter == 0)
					{
						break; // If we have a DataSize, we stop after processing that many characters
					}
					Counter--;
				}
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
				Ch >>= 8;
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
				Ch >>= 8;
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
				Ch >>= 8;
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
			}
			return ~CRC;
		}

		template <typename CharType>
		static typename TEnableIf<sizeof(CharType) == 1, uint32>::Type StrCrc32(const CharType* Data, uint32 CRC = 0, uint32 DataSize = 0)
		{
			/* Overload for when CharType is a byte, which causes warnings when right-shifting by 8 */
			CRC = ~CRC;
			uint32 Counter = DataSize;
			while (CharType Ch = *Data++)
			{
				if(DataSize > 0)
				{
					if (Counter == 0)
					{
						break; // If we have a DataSize, we stop after processing that many characters
					}
					Counter--;
				}
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC ^ Ch) & 0xFF];
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
				CRC = (CRC >> 8) ^ CRCTablesSB8[0][(CRC     ) & 0xFF];
			}
			return ~CRC;
		}
	};
}
