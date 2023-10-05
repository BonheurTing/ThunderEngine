#include "RHIHelper.h"

namespace Thunder
{
	DXGI_FORMAT ConvertRHIFormatToD3DFormat(RHIFormat type)
    {
    	return static_cast<DXGI_FORMAT>(type);
    }
}

