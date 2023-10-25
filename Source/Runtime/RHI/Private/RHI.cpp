#include "RHI.h"

namespace Thunder
{
	uint32 GetTypeHash(const RHIRasterizerState& Initializer)
	{/*
		uint32 Hash = GetTypeHash(Initializer.FillMode);
		Hash = HashCombine(Hash, GetTypeHash(Initializer.CullMode));
		Hash = HashCombine(Hash, GetTypeHash(Initializer.DepthBias));
		Hash = HashCombine(Hash, GetTypeHash(Initializer.SlopeScaleDepthBias));
		Hash = HashCombine(Hash, GetTypeHash(Initializer.bAllowMSAA));
		Hash = HashCombine(Hash, GetTypeHash(Initializer.bEnableLineAA));*/
		return 0;
	}
	
	bool operator== (const RHIRasterizerState& a, const RHIRasterizerState& b)
	{
		const bool bSame = 
			a.FillMode == b.FillMode && 
			a.CullMode == b.CullMode && 
			a.FrontCounterClockwise == b.FrontCounterClockwise && 
			a.DepthBias == b.DepthBias &&
			abs(a.DepthBiasClamp - b.DepthBiasClamp) < 1e-3f &&
			abs(a.SlopeScaleDepthBias - b.SlopeScaleDepthBias) < 1e-3f  &&
			a.DepthClipEnable == b.DepthClipEnable &&
			a.MultisampleEnable == b.MultisampleEnable &&
			a.AntialiasedLineEnable == b.AntialiasedLineEnable &&
			a.ForcedSampleCount == b.ForcedSampleCount &&
			a.ConservativeRaster == b.ConservativeRaster;
		return bSame;
	}

	
}
