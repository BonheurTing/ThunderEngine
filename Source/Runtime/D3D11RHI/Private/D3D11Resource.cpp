#include "D3D11Resource.h"

namespace Thunder
{
	UINT GetRHIResourceBindFlags(RHIResourceFlags flags)
	{
		UINT outFlags = 0;
		if (flags.NeedUAV)
		{
			outFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}
		if (flags.NeedDSV)
		{
			outFlags |= D3D11_BIND_DEPTH_STENCIL;
		}
		if (flags.NeedRTV)
		{
			outFlags |= D3D11_BIND_RENDER_TARGET;
		}
		if (outFlags == 0)
		{
			outFlags |= D3D11_BIND_SHADER_RESOURCE;
		}
		return outFlags;
	}
}
