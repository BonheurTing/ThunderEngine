#pragma once

#pragma warning(error:4834)

#define _NODISCARD_ [[nodiscard]]

namespace Thunder
{
	/* Unsafe Binary Data*/
	struct BinaryData
	{
		void* Data;
		size_t Size;
	};

	/* todo: Safe Binary Data*/
}