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
#include "../../RenderCore/Public/RenderPass.h"

namespace Thunder
{
#define DEBUG_DX12_PSO_CACHE 0

	void GetD3D12VertexDeclaration(const RHIVertexDeclarationDescriptor& desc, TArray<D3D12_INPUT_ELEMENT_DESC>& OutVertexElements)
	{
		for(const RHIVertexElement& element : desc.Elements)
		{
			D3D12_INPUT_ELEMENT_DESC d3dElement;
			TAssertf(GVertexInputSemanticToString.contains(element.Name), "Unknown vertex element semantic");
			d3dElement.SemanticName = GVertexInputSemanticToString.find(element.Name)->second.c_str();
			d3dElement.SemanticIndex = element.Index;
			d3dElement.Format = static_cast<DXGI_FORMAT>(element.Format);
			d3dElement.InputSlot = element.InputSlot;
			d3dElement.AlignedByteOffset = element.AlignedByteOffset;
			d3dElement.InputSlotClass = element.IsPerInstanceData ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			d3dElement.InstanceDataStepRate = 0;
			OutVertexElements.push_back(d3dElement);
		}
		TAssertf(desc.Elements.size() < MaxVertexElementCount, "Too many vertex elements in declaration");
	}
	
	void GetD3D12GraphicsPipelineStateDesc(const TGraphicsPipelineStateDescriptor& rhiDesc, TD3D12GraphicsPipelineStateDesc& outD3D12Desc)
	{
		memset(&(outD3D12Desc.Desc), 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

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

		uint32 const renderTargetCount = static_cast<uint32>(rhiDesc.Pass->GetRenderTargetCount());
		outD3D12Desc.Desc.NumRenderTargets = renderTargetCount;
		for (uint32 i = 0u; i < renderTargetCount; ++i)
		{
			outD3D12Desc.Desc.RTVFormats[i] = static_cast<DXGI_FORMAT>(rhiDesc.Pass->GetRenderTargetFormat(i));
		}
		outD3D12Desc.Desc.DSVFormat = static_cast<DXGI_FORMAT>(rhiDesc.Pass->GetDepthStencilFormat());
		outD3D12Desc.Desc.SampleDesc.Count = rhiDesc.NumSamples;
		outD3D12Desc.Desc.SampleDesc.Quality = 0;

		auto& shaderBound = rhiDesc.shaderVariant->GetShaders();
		auto CopyShader = [&shaderBound](D3D12_SHADER_BYTECODE& destination, EShaderStageType sourceStage)
		{
			if (shaderBound.contains(sourceStage))
			{
				const BinaryData* byteCode = &shaderBound[sourceStage]->ByteCode;
				destination.pShaderBytecode = byteCode->Data;
				destination.BytecodeLength = byteCode->Size;
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
		outD3D12Desc.CombinedHash = FCrc::BinaryCrc32(stateIdentifier.data(), static_cast<uint32>(stateIdentifier.size()), 0);
		uint32 shaderHash = ShaderCombination::GetTypeHash(*rhiDesc.shaderVariant);
		outD3D12Desc.CombinedHash = FCrc::BinaryCrc32(reinterpret_cast<const uint8*>(&shaderHash), 4, outD3D12Desc.CombinedHash);
	}
	
	TD3D12GraphicsPipelineState* TD3D12PipelineStateCache::CreateAndAddToCache(const TGraphicsPipelineStateDescriptor& rhiDesc, const TD3D12GraphicsPipelineStateDesc& d3d12Desc)
	{
		ComPtr<ID3D12PipelineState> pso;

		const HRESULT hr = ParentDevice->CreateGraphicsPipelineState(&d3d12Desc.Desc, IID_PPV_ARGS(&pso));
		if (SUCCEEDED(hr))
		{
			GraphicsPipelineStateCache[d3d12Desc] = MakeRefCount<TD3D12GraphicsPipelineState>(rhiDesc, pso);
			return GraphicsPipelineStateCache[d3d12Desc].Get();
		}
		else
		{
			TAssertf(false, "Failed to create graphics pipeline state");
			return nullptr;
		}
	}

	D3D12GraphicsPipelineStateRef TD3D12PipelineStateCache::CreateGraphicsPSO(const TGraphicsPipelineStateDescriptor& rhiDesc, const TD3D12GraphicsPipelineStateDesc& d3d12Desc) const
	{
		ComPtr<ID3D12PipelineState> pso;

		const HRESULT hr = ParentDevice->CreateGraphicsPipelineState(&d3d12Desc.Desc, IID_PPV_ARGS(&pso));
		if (SUCCEEDED(hr))
		{
			D3D12GraphicsPipelineStateRef psoRef = MakeRefCount<TD3D12GraphicsPipelineState>(rhiDesc, pso);
			return psoRef;
		}
		else
		{
			TAssertf(false, "Failed to create graphics pipeline state, error code : %d.", hr);
			return nullptr;
		}
	}

	uint64 HashPSODesc(const TGraphicsPipelineStateDescriptor& Desc)
	{
		//1.2todo HashPSOKey(shaderVariant->GetKey(), RenderState->GetKey(), RenderTargets, InputLayout->GetKey());
		return 0; 
	}

	uint64 HashPSODesc(const TComputePipelineStateDescriptor& Desc)
	{
		return 0;
	}

	TD3D12GraphicsPipelineState* TD3D12PipelineStateCache::FindOrCreateGraphicsPipelineState(const TGraphicsPipelineStateDescriptor& rhiDesc)
	{
		TD3D12GraphicsPipelineStateDesc d3d12Desc{};
		GetD3D12GraphicsPipelineStateDesc(rhiDesc, d3d12Desc);

    	// PSO map quick path.
	    {
			auto lock = CacheLock.Read();
	    	auto psoIt = GraphicsPipelineStateCache.find(d3d12Desc);
	    	if (psoIt != GraphicsPipelineStateCache.end())
	    	{
	    		return psoIt->second.Get();
	    	}
    	}

    	// Sync-compilation deduplication.
	    SyncCompilingGraphicsPSOEntry* syncCompilingEntry = nullptr;
	    {
    		auto syncCompilingMapLock = SyncCompilingPSOMapLock.Read();
	    	auto psoIt = SyncCompilingPSOMap.find(d3d12Desc);
	    	if (psoIt != SyncCompilingPSOMap.end()) [[unlikely]]
	    	{
	    		// Already compiling, fetch compiling status.
	    		syncCompilingEntry = SyncCompilingPSOMap[d3d12Desc];
	    	}
	    }

    	// Deduplication hit.
    	if (syncCompilingEntry) [[unlikely]]
    	{
    		// Wait for sync compilation thread finishing compiling.
    		auto psoLockGuard = syncCompilingEntry->Lock.Read();
    		return syncCompilingEntry->GraphicsPSO;
    	}

    	// Deduplication miss.
	    SyncCompilingPSOMapLock.WriteLock();
	    {
    		// Check again.
	    	auto psoIt = SyncCompilingPSOMap.find(d3d12Desc);
	    	if (psoIt != SyncCompilingPSOMap.end()) [[unlikely]]
	    	{
	    		// Already compiling, fetch compiling status.
	    		syncCompilingEntry = SyncCompilingPSOMap[d3d12Desc];
	    		SyncCompilingPSOMapLock.WriteUnlock();
	    		
	    		// Wait for sync compilation thread finishing compiling.
	    		auto psoLockGuard = syncCompilingEntry->Lock.Read();
	    		return syncCompilingEntry->GraphicsPSO;
	    	}
	    }

    	// Launch a compilation task.
    	syncCompilingEntry = new (TMemory::Malloc<SyncCompilingGraphicsPSOEntry>()) SyncCompilingGraphicsPSOEntry;
    	SyncCompilingPSOMap[d3d12Desc] = syncCompilingEntry;
    	syncCompilingEntry->Lock.WriteLock();
    	SyncCompilingPSOMapLock.WriteUnlock();
		auto psoRef = CreateGraphicsPSO(rhiDesc, d3d12Desc); // Compile a PSO.
    	syncCompilingEntry->GraphicsPSO = psoRef;
    	syncCompilingEntry->Lock.WriteUnlock();

    	// Write back to pso cache.
	    auto psoCacheLock = CacheLock.Write();
    	TAssertf(!GraphicsPipelineStateCache.contains(d3d12Desc), "Duplicated pso found.");
    	GraphicsPipelineStateCache[d3d12Desc] = psoRef;

    	return psoRef.Get();
	}

}
