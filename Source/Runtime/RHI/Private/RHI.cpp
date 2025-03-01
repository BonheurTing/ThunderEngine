#include "RHI.h"

namespace Thunder
{
	THashMap<ERHIDepthBiasType, DepthBiasConfig> GRHIDepthBiasConfig
	{
		{ ERHIDepthBiasType::Default, {} },
		{ ERHIDepthBiasType::Invalid, {} }
	};

	RHIBlendDescriptor::RHIBlendDescriptor() :  // NOLINT(cppcoreguidelines-pro-type-member-init)
		BlendEnable(0),
		LogicOpEnable(0),
		SrcBlend(ERHIBlend::One),
		DestBlend(ERHIBlend::Zero),
		BlendOp(ERHIBlendOp::Add),
		SrcBlendAlpha(ERHIBlend::One),
		DestBlendAlpha(ERHIBlend::Zero),
		BlendOpAlpha(ERHIBlendOp::Add),
		LogicOp(ERHILogicOp::Noop),
		RenderTargetWriteMask(0x0F),
		Padding(0)
	{
	}

	RHIBlendState::RHIBlendState() :  // NOLINT(cppcoreguidelines-pro-type-member-init)
		RenderTarget(),
		AlphaToCoverageEnable(0),
		IndependentBlendEnable(0),
		Padding(0)
	{
	}

	RHIRasterizerState::RHIRasterizerState() :  // NOLINT(cppcoreguidelines-pro-type-member-init)
		FillMode(ERHIFillMode::Solid),
		CullMode(ERHICullMode::Back),
		FrontCounterClockwise(0),
		DepthBias(ERHIDepthBiasType::Default),
		DepthClipEnable(1),
		MultisampleEnable(0),
		AntiAliasedLineEnable(0),
		ForcedSampleCount(0),
		ConservativeRaster(0),
		Padding(0)
	{
	}

	RHIDepthStencilState::RHIDepthStencilState() :
		DepthEnable(1),
		DepthWriteMask(ERHIDepthWriteMask::All),
		DepthFunc(ERHIComparisonFunc::Less),
		StencilEnable(0),
		StencilReadMask(0xff),
		StencilWriteMask(0xff),
		FrontFaceStencilFailOp(ERHIStencilOp::Keep),
		FrontFaceStencilDepthFailOp(ERHIStencilOp::Keep),
		FrontFaceStencilPassOp(ERHIStencilOp::Keep),
		FrontFaceStencilFunc(ERHIComparisonFunc::Always),
		BackFaceStencilFailOp(ERHIStencilOp::Keep),
		BackFaceStencilDepthFailOp(ERHIStencilOp::Keep),
		BackFaceStencilPassOp(ERHIStencilOp::Keep),
		BackFaceStencilFunc(ERHIComparisonFunc::Always)
	{
	}

	void TGraphicsPipelineStateDescriptor::GetStateIdentifier(TArray<uint8>& outIdentifier) const
	{	
		for (const auto& registerCount : RegisterCounts.Hash)
		{
			outIdentifier.push_back(registerCount);
		}
		outIdentifier.push_back(static_cast<uint8>(VertexDeclaration.Elements.size()));
		for (const auto& vertexElement : VertexDeclaration.Elements)
		{
			for (const auto& hash : vertexElement.Hash)
			{
				outIdentifier.push_back(hash);
			}
		}
		for (const auto& hash : BlendState.Hash)
		{
			outIdentifier.push_back(hash);
		}
		for (const auto& hash : RasterizerState.Hash)
		{
			outIdentifier.push_back(hash);
		}
		for (const auto& hash : DepthStencilState.Hash)
		{
			outIdentifier.push_back(hash);
		}
	}
}
