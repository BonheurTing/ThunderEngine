#pragma once
#include "RHIResource.h"
#include "RHIDefinition.h"
#include "d3d12.h"

namespace Thunder
{
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
			depthStencilDesc.DepthEnable,
			static_cast<D3D12_DEPTH_WRITE_MASK>(depthStencilDesc.DepthWriteMask),
			static_cast<D3D12_COMPARISON_FUNC>(depthStencilDesc.DepthFunc),
			depthStencilDesc.StencilEnable,
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
