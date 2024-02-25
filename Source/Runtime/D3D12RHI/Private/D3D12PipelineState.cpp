#pragma optimize("", off)
#include "D3D12PipelineState.h"
#include "CRC.h"
#include "D3D12Resource.h"
#include "D3D12RHIModule.h"
#include "D3D12RootSignature.h"
#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "ShaderDefinition.h"
#include "d3dx12.h"
#include "ShaderModule.h"

namespace Thunder
{
#define DEBUG_DX12_PSO_CACHE 0

	void GetD3D12VertexDeclaration(const RHIVertexDeclarationDescriptor& desc, TArray<D3D12_INPUT_ELEMENT_DESC>& OutVertexElements)
	{
		for(const RHIVertexElement& element : desc.Elements)
		{
			D3D12_INPUT_ELEMENT_DESC D3DElement;
			TAssertf(GVertexInputSemanticToString.contains(element.Name), "Unknown vertex element semantic");
			D3DElement.SemanticName = GVertexInputSemanticToString.find(element.Name)->second.c_str();
			D3DElement.SemanticIndex = element.Index;
			D3DElement.Format = static_cast<DXGI_FORMAT>(element.Format);
			D3DElement.InputSlot = element.InputSlot;
			D3DElement.AlignedByteOffset = element.AlignedByteOffset;
			D3DElement.InputSlotClass = element.IsPerInstanceData ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			D3DElement.InstanceDataStepRate = 0;
			OutVertexElements.push_back(D3DElement);
		}
		TAssertf(desc.Elements.size() < MaxVertexElementCount, "Too many vertex elements in declaration");
	}
	
	void GetD3D12GraphicsPipelineStateDesc(const TGraphicsPipelineStateDescriptor& rhiDesc, TD3D12GraphicsPipelineStateDesc& outD3D12Desc)
	{
		memset(&outD3D12Desc, 0, sizeof(TD3D12GraphicsPipelineStateDesc));
		
		const TD3D12RootSignature* rootSignature = TD3D12RHIModule::GetModule()->GetRootSignatureManager()->GetRootSignature(rhiDesc.RegisterCounts);
		outD3D12Desc.Desc.pRootSignature = rootSignature->GetRootSignature();
		outD3D12Desc.Desc.StreamOutput = {nullptr, 0, nullptr , 0, 0};
		outD3D12Desc.Desc.BlendState = CastD3D12BlendStateFromRHI(rhiDesc.BlendState);
		outD3D12Desc.Desc.SampleMask = 0xFFFFFFFF;
		outD3D12Desc.Desc.RasterizerState = CastD3D12RasterizerFromRHI(rhiDesc.RasterizerState);
		outD3D12Desc.Desc.DepthStencilState = CastD3D12DepthStencilSFromRHI(rhiDesc.DepthStencilState);
		outD3D12Desc.Desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		outD3D12Desc.Desc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(rhiDesc.PrimitiveType);

		GetD3D12VertexDeclaration(rhiDesc.VertexDeclaration, outD3D12Desc.InputLayout);
		outD3D12Desc.Desc.InputLayout.NumElements = static_cast<UINT>(outD3D12Desc.InputLayout.size());
		outD3D12Desc.Desc.InputLayout.pInputElementDescs = outD3D12Desc.InputLayout.data();
		
		if (rhiDesc.RenderTargetsEnabled)
		{
			outD3D12Desc.Desc.NumRenderTargets = 0;
			for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
			{
				if (rhiDesc.RenderTargetFormats[i] != RHIFormat::UNKNOWN)
				{
					outD3D12Desc.Desc.NumRenderTargets++;
					outD3D12Desc.Desc.RTVFormats[i] = static_cast<DXGI_FORMAT>(rhiDesc.RenderTargetFormats[i]);
					continue;
				}
				break;
			}
		}
		outD3D12Desc.Desc.DSVFormat = static_cast<DXGI_FORMAT>(rhiDesc.DepthStencilFormat);
		outD3D12Desc.Desc.SampleDesc.Count = rhiDesc.NumSamples;
		outD3D12Desc.Desc.SampleDesc.Quality = 0;
		
		ShaderModule* shaderModule = ShaderModule::GetModule();
		ShaderCombination* combination = shaderModule->GetShaderCombination(rhiDesc.ShaderIdentifier.ToString());
		TAssertf(combination != nullptr, "Failed to get shader combination");
		auto& shaderBound = combination->Shaders;
		auto CopyShader = [&shaderBound](D3D12_SHADER_BYTECODE& destination, EShaderStageType sourceStage)
		{
			if (shaderBound.contains(sourceStage))
			{
				const auto& byteCode = shaderBound[sourceStage].ByteCode;
				destination.pShaderBytecode = byteCode.Data;
				destination.BytecodeLength = byteCode.Size;
			}
		};
		CopyShader(outD3D12Desc.Desc.VS, EShaderStageType::Vertex);
		CopyShader(outD3D12Desc.Desc.PS, EShaderStageType::Pixel);
		CopyShader(outD3D12Desc.Desc.DS, EShaderStageType::Domain);
		CopyShader(outD3D12Desc.Desc.HS, EShaderStageType::Hull);
		CopyShader(outD3D12Desc.Desc.GS, EShaderStageType::Geometry);
		
		outD3D12Desc.Desc.Flags = DEBUG_DX12_PSO_CACHE ? D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG : D3D12_PIPELINE_STATE_FLAG_NONE;

		TArray<uint8> stateIdentifier;
		rhiDesc.GetStateIdentifier(stateIdentifier);
		outD3D12Desc.CombinedHash = FCrc::StrCrc32(stateIdentifier.data());
		outD3D12Desc.CombinedHash = FCrc::StrCrc32(rhiDesc.ShaderIdentifier.c_str(), outD3D12Desc.CombinedHash);
	}
	
	TD3D12GraphicsPipelineState* TD3D12PipelineStateCache::CreateAndAddToCache(const TGraphicsPipelineStateDescriptor& rhiDesc, const TD3D12GraphicsPipelineStateDesc& d3d12Desc)
	{
		ComPtr<ID3D12PipelineState> pso;
		
		const HRESULT hr = ParentDevice->CreateGraphicsPipelineState(&d3d12Desc.Desc, IID_PPV_ARGS(&pso));
		if (SUCCEEDED(hr))
		{
			GraphicsPipelineStateCache[d3d12Desc] = MakeRefCount<TD3D12GraphicsPipelineState>(rhiDesc, pso);
			return GraphicsPipelineStateCache[d3d12Desc].get();
		}
		else
		{
			TAssertf(false, "Failed to create graphics pipeline state");
			return nullptr;
		}
	}

	TD3D12GraphicsPipelineState* TD3D12PipelineStateCache::FindOrCreateGraphicsPipelineState(const TGraphicsPipelineStateDescriptor& rhiDesc)
	{
		TD3D12GraphicsPipelineStateDesc d3d12Desc{};
		GetD3D12GraphicsPipelineStateDesc(rhiDesc, d3d12Desc);

		if (GraphicsPipelineStateCache.contains(d3d12Desc))
		{
			return GraphicsPipelineStateCache[d3d12Desc].get();
		}
		else
		{
			return CreateAndAddToCache(rhiDesc, d3d12Desc);
		}
	}

}
#pragma optimize("", on)