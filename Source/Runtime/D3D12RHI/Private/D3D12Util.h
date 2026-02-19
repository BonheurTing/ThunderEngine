#pragma once
#include "RHIResource.h"
#include "RHIDefinition.h"
#include "d3d12.h"

namespace Thunder
{
    inline D3D12_SRV_DIMENSION ConvertToD3D12SRVDimension(ERHIViewDimension dim)
    {
        switch (dim)
        {
        case ERHIViewDimension::Buffer:           return D3D12_SRV_DIMENSION_BUFFER;
        case ERHIViewDimension::Texture1D:        return D3D12_SRV_DIMENSION_TEXTURE1D;
        case ERHIViewDimension::Texture1DArray:   return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        case ERHIViewDimension::Texture2D:        return D3D12_SRV_DIMENSION_TEXTURE2D;
        case ERHIViewDimension::Texture2DArray:   return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        case ERHIViewDimension::Texture2DMS:      return D3D12_SRV_DIMENSION_TEXTURE2DMS;
        case ERHIViewDimension::Texture2DMSArray: return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        case ERHIViewDimension::Texture3D:        return D3D12_SRV_DIMENSION_TEXTURE3D;
        case ERHIViewDimension::TextureCube:      return D3D12_SRV_DIMENSION_TEXTURECUBE;
        case ERHIViewDimension::TextureCubeArray: return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        default:                                  return D3D12_SRV_DIMENSION_UNKNOWN;
        }
    }

    inline D3D12_UAV_DIMENSION ConvertToD3D12UAVDimension(ERHIViewDimension dim)
    {
        switch (dim)
        {
        case ERHIViewDimension::Buffer:          return D3D12_UAV_DIMENSION_BUFFER;
        case ERHIViewDimension::Texture1D:       return D3D12_UAV_DIMENSION_TEXTURE1D;
        case ERHIViewDimension::Texture1DArray:  return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        case ERHIViewDimension::Texture2D:       return D3D12_UAV_DIMENSION_TEXTURE2D;
        case ERHIViewDimension::Texture2DArray:  return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        case ERHIViewDimension::Texture3D:       return D3D12_UAV_DIMENSION_TEXTURE3D;
        default:                                 return D3D12_UAV_DIMENSION_UNKNOWN;
        }
    }

    inline D3D12_RTV_DIMENSION ConvertToD3D12RTVDimension(ERHIViewDimension dim)
    {
        switch (dim)
        {
        case ERHIViewDimension::Buffer:           return D3D12_RTV_DIMENSION_BUFFER;
        case ERHIViewDimension::Texture1D:        return D3D12_RTV_DIMENSION_TEXTURE1D;
        case ERHIViewDimension::Texture1DArray:   return D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        case ERHIViewDimension::Texture2D:        return D3D12_RTV_DIMENSION_TEXTURE2D;
        case ERHIViewDimension::Texture2DArray:   return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        case ERHIViewDimension::Texture2DMS:      return D3D12_RTV_DIMENSION_TEXTURE2DMS;
        case ERHIViewDimension::Texture2DMSArray: return D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
        case ERHIViewDimension::Texture3D:        return D3D12_RTV_DIMENSION_TEXTURE3D;
        default:                                  return D3D12_RTV_DIMENSION_UNKNOWN;
        }
    }

    inline D3D12_DSV_DIMENSION ConvertToD3D12DSVDimension(ERHIViewDimension dim)
    {
        switch (dim)
        {
        case ERHIViewDimension::Texture1D:        return D3D12_DSV_DIMENSION_TEXTURE1D;
        case ERHIViewDimension::Texture1DArray:   return D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        case ERHIViewDimension::Texture2D:        return D3D12_DSV_DIMENSION_TEXTURE2D;
        case ERHIViewDimension::Texture2DArray:   return D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        case ERHIViewDimension::Texture2DMS:      return D3D12_DSV_DIMENSION_TEXTURE2DMS;
        case ERHIViewDimension::Texture2DMSArray: return D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        default:                                  return D3D12_DSV_DIMENSION_UNKNOWN;
        }
    }


	inline D3D12_BLEND_DESC CastD3D12BlendStateFromRHI(const RHIBlendState& blendDesc)
	{
		D3D12_BLEND_DESC desc
		{
			blendDesc.AlphaToCoverageEnable,
			blendDesc.IndependentBlendEnable,
			{{}}
		};
		for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
		{
			desc.RenderTarget[i].BlendEnable = blendDesc.RenderTarget[i].BlendEnable;
			desc.RenderTarget[i].LogicOpEnable = blendDesc.RenderTarget[i].LogicOpEnable;
			desc.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(blendDesc.RenderTarget[i].SrcBlend);
			desc.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(blendDesc.RenderTarget[i].DestBlend);
			desc.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(blendDesc.RenderTarget[i].BlendOp);
			desc.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(blendDesc.RenderTarget[i].SrcBlendAlpha);
			desc.RenderTarget[i].DestBlendAlpha = static_cast<D3D12_BLEND>(blendDesc.RenderTarget[i].DestBlendAlpha);
			desc.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(blendDesc.RenderTarget[i].BlendOpAlpha);
			desc.RenderTarget[i].LogicOp = static_cast<D3D12_LOGIC_OP>(blendDesc.RenderTarget[i].LogicOp);
			desc.RenderTarget[i].RenderTargetWriteMask = blendDesc.RenderTarget[i].RenderTargetWriteMask;
		}
		return desc;
	}

	inline D3D12_RASTERIZER_DESC CastD3D12RasterizerFromRHI(const RHIRasterizerState& rasterizerDesc)
	{
		const D3D12_RASTERIZER_DESC desc
		{
			static_cast<D3D12_FILL_MODE>(rasterizerDesc.FillMode),
			static_cast<D3D12_CULL_MODE>(rasterizerDesc.CullMode),
			rasterizerDesc.FrontCounterClockwise,
			GRHIDepthBiasConfig[rasterizerDesc.DepthBias].Bias,
			GRHIDepthBiasConfig[rasterizerDesc.DepthBias].BiasClamp,
			GRHIDepthBiasConfig[rasterizerDesc.DepthBias].SlopeScaledBias,
			rasterizerDesc.DepthClipEnable,
			rasterizerDesc.MultisampleEnable,
			rasterizerDesc.AntiAliasedLineEnable,
			rasterizerDesc.ForcedSampleCount,
			static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(rasterizerDesc.ConservativeRaster)
		};
		return desc;
	}

	inline D3D12_DEPTH_STENCIL_DESC CastD3D12DepthStencilSFromRHI(const RHIDepthStencilState& depthStencilDesc)
	{
		D3D12_DEPTH_STENCIL_DESC desc
		{
			(depthStencilDesc.DepthEnable > 0),
			static_cast<D3D12_DEPTH_WRITE_MASK>(depthStencilDesc.DepthWriteMask),
			static_cast<D3D12_COMPARISON_FUNC>(static_cast<uint8>(depthStencilDesc.DepthFunc) + 1),
			(depthStencilDesc.StencilEnable > 0),
			depthStencilDesc.StencilReadMask,
			depthStencilDesc.StencilWriteMask,
			{
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.FrontFaceStencilFailOp) + 1),
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.FrontFaceStencilDepthFailOp) + 1),
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.FrontFaceStencilPassOp) + 1),
				static_cast<D3D12_COMPARISON_FUNC>(static_cast<uint8>(depthStencilDesc.FrontFaceStencilFunc) + 1)
			},
			{
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.BackFaceStencilFailOp) + 1),
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.BackFaceStencilDepthFailOp) + 1),
				static_cast<D3D12_STENCIL_OP>(static_cast<uint8>(depthStencilDesc.BackFaceStencilPassOp) + 1),
				static_cast<D3D12_COMPARISON_FUNC>(static_cast<uint8>(depthStencilDesc.BackFaceStencilFunc) + 1)
			}
		};
		return desc;
	}

}
