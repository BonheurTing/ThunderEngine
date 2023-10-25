#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "ShaderDefinition.h"
#include "d3dx12.h"

namespace Thunder
{
#define DEBUG_DX12_PSO_CACHE 0
	
	TD3D12GraphicsPipelineStateDesc GetGraphicsPipelineStateDesc(const TGraphicsPipelineStateInitializer& initializer, const TD3D12RootSignature* rootSignature)
	{
		TD3D12GraphicsPipelineStateDesc pipelineStateDesc {};
		memset(&pipelineStateDesc, 0, sizeof(pipelineStateDesc));
		
		pipelineStateDesc.Desc.pRootSignature = rootSignature->GetRootSignature();
		
		pipelineStateDesc.Desc.StreamOutput = {nullptr, 0, nullptr , 0, 0};
		pipelineStateDesc.Desc.BlendState = initializer.BlendState ? CastD3D12BlendStateFromRHI(initializer.BlendState) : CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pipelineStateDesc.Desc.SampleMask = 0xFFFFFFFF;
		pipelineStateDesc.Desc.RasterizerState = initializer.RasterizerState ? CastD3D12RasterizerFromRHI(initializer.RasterizerState) : CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDesc.Desc.DepthStencilState = initializer.DepthStencilState ? CastD3D12DepthStencilSFromRHI(initializer.DepthStencilState) : CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pipelineStateDesc.Desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		pipelineStateDesc.Desc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(initializer.PrimitiveType);
		
		if (initializer.RenderTargetsEnabled)
		{
			pipelineStateDesc.Desc.NumRenderTargets = 0;
			for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
			{
				if (initializer.RenderTargetFormats[i] != RHIFormat::UNKNOWN)
				{
					pipelineStateDesc.Desc.NumRenderTargets++;
					pipelineStateDesc.Desc.RTVFormats[i] = static_cast<DXGI_FORMAT>(initializer.RenderTargetFormats[i]);
					continue;
				}
				break;
			}
		}
		pipelineStateDesc.Desc.DSVFormat = static_cast<DXGI_FORMAT>(initializer.DepthStencilFormat);
		pipelineStateDesc.Desc.SampleDesc.Count = initializer.NumSamples;
		pipelineStateDesc.Desc.SampleDesc.Quality = 0;

		if (auto InputLayout = static_cast<D3D12RHIVertexDeclaration*>(initializer.VertexDeclaration))
		{
			pipelineStateDesc.Desc.InputLayout.NumElements = static_cast<UINT>(InputLayout->VertexElements.size());
			pipelineStateDesc.Desc.InputLayout.pInputElementDescs = InputLayout->VertexElements.data();
		}

		auto shaderBound = initializer.ShaderCombination->Shaders;
		#define COPY_SHADER(Shader, StageName) \
			if (shaderBound.contains(EShaderStageType::StageName)) \
			{ \
				auto byteCode = shaderBound[EShaderStageType::StageName].ByteCode; \
				pipelineStateDesc.Desc.Shader.pShaderBytecode = byteCode.Data; \
				pipelineStateDesc.Desc.Shader.BytecodeLength = byteCode.Size; \
			}
			COPY_SHADER(VS, Vertex)
			COPY_SHADER(PS, Pixel)
			COPY_SHADER(DS, Domain)
			COPY_SHADER(HS, Hull)
			COPY_SHADER(GS, Geometry)
		#undef COPY_SHADER
		
		pipelineStateDesc.Desc.Flags = DEBUG_DX12_PSO_CACHE ? D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG : D3D12_PIPELINE_STATE_FLAG_NONE;
		return pipelineStateDesc;
	}
	
	D3D12PipelineState* TD3D12PipelineStateCache::CreateAndAddToCache(const TD3D12GraphicsPipelineStateDesc& desc)
	{
		ComPtr<ID3D12PipelineState> pso;
		HRESULT hr = ParentDevice->CreateGraphicsPipelineState(&desc.Desc, IID_PPV_ARGS(&pso));
		if (SUCCEEDED(hr))
		{
			const auto newPSO = new D3D12PipelineState{pso.Get()};
			GraphicsPipelineStateCache[desc] = newPSO;
			return newPSO;
		}
		else
		{
			TAssertf(false, "Failed to create graphics pipeline state");
			return nullptr;
		}
	}

	TD3D12GraphicsPipelineState* TD3D12PipelineStateCache::FindInLoadedCache(const TGraphicsPipelineStateInitializer& initializer,
		const TD3D12RootSignature* rootSignature, TD3D12GraphicsPipelineStateDesc& outDesc)
	{
		outDesc = GetGraphicsPipelineStateDesc(initializer, rootSignature);

		if (GraphicsPipelineStateCache.contains(outDesc))
		{
			return new TD3D12GraphicsPipelineState(initializer, GraphicsPipelineStateCache[outDesc]);
		}
		return nullptr;
	}

}
