#include "RHIHelper.h"
#include "RHIDefinition.h"

DXGI_FORMAT ConvertRHIFormatToD3DFormat(RHIFormat type)
{
	return static_cast<DXGI_FORMAT>(type);
}

D3D12_RESOURCE_FLAGS GetRHIResourceFlags(RHIResourceFlags flags)
{
	D3D12_RESOURCE_FLAGS outFlags = D3D12_RESOURCE_FLAG_NONE;
	if(!flags.NeedSRV)
	{
		outFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}
	if(flags.NeedUAV)
	{
		outFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	if(flags.NeedDSV)
	{
		outFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if(flags.NeedRTV)
	{
		outFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	return outFlags;
}
