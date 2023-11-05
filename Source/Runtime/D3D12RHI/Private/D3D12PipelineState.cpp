#include "D3D12PipelineState.h"

#include "CRC.h"
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "ShaderDefinition.h"
#include "d3dx12.h"

namespace Thunder
{
#define DEBUG_DX12_PSO_CACHE 0
	
	void GetGraphicsPipelineStateDesc(const TGraphicsPipelineStateInitializer& initializer, const TD3D12RootSignature* rootSignature, TD3D12GraphicsPipelineStateDesc& outDesc)
	{
		memset(&outDesc, 0, sizeof(TD3D12GraphicsPipelineStateDesc));

		outDesc.Desc.pRootSignature = rootSignature->GetRootSignature();
		outDesc.Desc.StreamOutput = {nullptr, 0, nullptr , 0, 0};
		outDesc.Desc.BlendState = initializer.BlendState ? CastD3D12BlendStateFromRHI(initializer.BlendState) : CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		outDesc.Desc.SampleMask = 0xFFFFFFFF;
		outDesc.Desc.RasterizerState = initializer.RasterizerState ? CastD3D12RasterizerFromRHI(initializer.RasterizerState) : CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		outDesc.Desc.DepthStencilState = initializer.DepthStencilState ? CastD3D12DepthStencilSFromRHI(initializer.DepthStencilState) : CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		outDesc.Desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		outDesc.Desc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(initializer.PrimitiveType);

		if (initializer.RenderTargetsEnabled)
		{
			outDesc.Desc.NumRenderTargets = 0;
			for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
			{
				if (initializer.RenderTargetFormats[i] != RHIFormat::UNKNOWN)
				{
					outDesc.Desc.NumRenderTargets++;
					outDesc.Desc.RTVFormats[i] = static_cast<DXGI_FORMAT>(initializer.RenderTargetFormats[i]);
					continue;
				}
				break;
			}
		}
		outDesc.Desc.DSVFormat = static_cast<DXGI_FORMAT>(initializer.DepthStencilFormat);
		outDesc.Desc.SampleDesc.Count = initializer.NumSamples;
		outDesc.Desc.SampleDesc.Quality = 0;

		if (auto InputLayout = static_cast<D3D12RHIVertexDeclaration*>(initializer.VertexDeclaration))
		{
			outDesc.Desc.InputLayout.NumElements = static_cast<UINT>(InputLayout->VertexElements.size());
			outDesc.Desc.InputLayout.pInputElementDescs = InputLayout->VertexElements.data();
		}

		auto& shaderBound = initializer.ShaderCombination->Shaders;
		auto CopyShader = [&shaderBound](D3D12_SHADER_BYTECODE& destination, EShaderStageType sourceStage)
		{
			if (shaderBound.contains(sourceStage))
			{
				auto& byteCode = shaderBound[sourceStage].ByteCode;
				destination.pShaderBytecode = byteCode.GetData();
				destination.BytecodeLength = byteCode.GetSize();
			}
		};
		CopyShader(outDesc.Desc.VS, EShaderStageType::Vertex);
		CopyShader(outDesc.Desc.PS, EShaderStageType::Pixel);
		CopyShader(outDesc.Desc.DS, EShaderStageType::Domain);
		CopyShader(outDesc.Desc.HS, EShaderStageType::Hull);
		CopyShader(outDesc.Desc.GS, EShaderStageType::Geometry);
		
		outDesc.Desc.Flags = DEBUG_DX12_PSO_CACHE ? D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG : D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	
	D3D12PipelineState* TD3D12PipelineStateCache::CreateAndAddToCache(const TD3D12GraphicsPipelineStateDesc& desc)
	{
		ComPtr<ID3D12PipelineState> pso;
		
		const HRESULT hr = ParentDevice->CreateGraphicsPipelineState(&desc.Desc, IID_PPV_ARGS(&pso));
		if (SUCCEEDED(hr))
		{
			const auto newPSO = new D3D12PipelineState{pso.Get()}; // TODO
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
		GetGraphicsPipelineStateDesc(initializer, rootSignature, outDesc);
		
		if (GraphicsPipelineStateCache.contains(outDesc))
		{
			return new TD3D12GraphicsPipelineState(initializer, GraphicsPipelineStateCache[outDesc]); // TODO
		}
		return nullptr;
	}

}
