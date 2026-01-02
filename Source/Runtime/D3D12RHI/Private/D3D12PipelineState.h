#pragma once
#include "d3d12.h"
#include "CoreMinimal.h"
#include "RHIResource.h"
#include "D3D12RHICommon.h"

namespace Thunder
{
	class TD3D12RootSignature;
	struct TD3D12GraphicsPipelineStateDesc
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
		TArray<D3D12_INPUT_ELEMENT_DESC> InputLayout{};
		
		uint32 CombinedHash = 0;
		
		explicit TD3D12GraphicsPipelineStateDesc() {}
		bool operator==(const TD3D12GraphicsPipelineStateDesc& rhs) const
		{
			return CombinedHash == rhs.CombinedHash;
		}
	};

	struct TD3D12ComputePipelineStateDesc
	{
		//const FD3D12RootSignature *pRootSignature;
		D3D12_COMPUTE_PIPELINE_STATE_DESC Desc{};
		uint32 CSHash = 0;

		uint32 CombinedHash = 0;
	};
}
namespace std
{
	template<>
	struct hash<Thunder::TD3D12GraphicsPipelineStateDesc>
	{
		size_t operator()(const Thunder::TD3D12GraphicsPipelineStateDesc& value) const
		{
			return hash<Thunder::uint32>()(value.CombinedHash);
		}
	};

	template<>
	struct hash<Thunder::TD3D12ComputePipelineStateDesc>
	{
		size_t operator()(const Thunder::TD3D12ComputePipelineStateDesc& value) const
		{
			return hash<Thunder::uint32>()(value.CombinedHash);
		}
	};
}

namespace Thunder
{
	void GetD3D12GraphicsPipelineStateDesc(const TGraphicsPipelineStateDescriptor& descriptor, const TD3D12RootSignature* rootSignature, TD3D12GraphicsPipelineStateDesc& outDesc);
	//TD3D12ComputePipelineStateDesc GetComputePipelineStateDesc(const FD3D12ComputeShader* ComputeShader);
	
	class TD3D12GraphicsPipelineState : public TRHIGraphicsPipelineState
	{
	public:
		TD3D12GraphicsPipelineState(const TGraphicsPipelineStateDescriptor& desc, ComPtr<ID3D12PipelineState> inPipelineState)
			: TRHIGraphicsPipelineState(desc), PipelineState(inPipelineState) {}
		TD3D12GraphicsPipelineState(const TD3D12GraphicsPipelineState& rhs) = default;
		~TD3D12GraphicsPipelineState() = default;
		_NODISCARD_ void* GetPipelineState() const override { return PipelineState.Get(); }
		
	private:
		ComPtr<ID3D12PipelineState> PipelineState = nullptr;
	};

	class TD3D12ComputePipelineState : public TRHIComputePipelineState
	{
	public:
		TD3D12ComputePipelineState(const TComputePipelineStateDescriptor& desc, ComPtr<ID3D12PipelineState> inPipelineState)
			: TRHIComputePipelineState(desc), PipelineState(inPipelineState) {}
		TD3D12ComputePipelineState(const TD3D12ComputePipelineState& rhs) = default;
		~TD3D12ComputePipelineState() = default;
		_NODISCARD_ void* GetPipelineState() const override { return PipelineState.Get(); }
		
	private:
		ComPtr<ID3D12PipelineState> PipelineState = nullptr;
	};

	class TD3D12PipelineStateCache : public TD3D12DeviceChild
	{
	public:
		TD3D12PipelineStateCache(ID3D12Device* InDevice) : TD3D12DeviceChild(InDevice) {}
		TD3D12PipelineStateCache() = delete;
		
		TD3D12GraphicsPipelineState* FindOrCreateGraphicsPipelineState(const TGraphicsPipelineStateDescriptor& rhiDesc);
		
	private:
		TD3D12GraphicsPipelineState* CreateAndAddToCache(const TGraphicsPipelineStateDescriptor& rhiDesc, const TD3D12GraphicsPipelineStateDesc& d3d12Desc);

		THashMap<TD3D12GraphicsPipelineStateDesc, TRefCountPtr<TD3D12GraphicsPipelineState>> GraphicsPipelineStateCache;
		//HashMap<TD3D12ComputePipelineStateDesc, TD3D12ComputePipelineState> ComputePipelineStateCache;
	};

	static uint64 HashPSODesc(const TGraphicsPipelineStateDescriptor& Desc);
	static uint64 HashPSODesc(const TComputePipelineStateDescriptor& Desc);
	
}

