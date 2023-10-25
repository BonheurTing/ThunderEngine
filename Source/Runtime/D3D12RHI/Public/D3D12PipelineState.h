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
		//const FD3D12RootSignature *pRootSignature;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc;
		uint32 VSHash;
		uint32 MSHash;
		uint32 ASHash;
		uint32 GSHash;
		uint32 PSHash;
		uint32 InputLayoutHash;
		//bool bFromPSOFileCache;

		uint32 CombinedHash;

		bool operator==(const TD3D12GraphicsPipelineStateDesc& rhs) const
		{
			return CombinedHash == rhs.CombinedHash;
		}
	};

	struct TD3D12ComputePipelineStateDesc
	{
		//const FD3D12RootSignature *pRootSignature;
		D3D12_COMPUTE_PIPELINE_STATE_DESC Desc;
		uint32 CSHash;
		
		//bool bFromPSOFileCache;

		uint32 CombinedHash;
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
}

namespace Thunder
{
	//TD3D12GraphicsPipelineStateDesc GetGraphicsPipelineStateDesc(const TGraphicsPipelineStateInitializer& initializer, const TD3D12RootSignature* rootSignature);
	//TD3D12ComputePipelineStateDesc GetComputePipelineStateDesc(const FD3D12ComputeShader* ComputeShader);
	
	class D3D12PipelineState
	{
	public:
		explicit D3D12PipelineState(ID3D12PipelineState* inPso) : PipelineState(inPso) {}
		~D3D12PipelineState() = default;

		bool IsValid() {return false;}

	private:
		RefCountPtr<ID3D12PipelineState> PipelineState;
	
	};
	
	struct TD3D12GraphicsPipelineState : public TRHIGraphicsPipelineState
	{
		TD3D12GraphicsPipelineState(const TGraphicsPipelineStateInitializer& initializer, D3D12PipelineState* inPipelineState)
			: PipelineStateInitializer(initializer), PipelineState(inPipelineState) {}
		TD3D12GraphicsPipelineState(const TD3D12GraphicsPipelineState& rhs) = default;
		~TD3D12GraphicsPipelineState() = default;
		
		TGraphicsPipelineStateInitializer PipelineStateInitializer;
		D3D12PipelineState* PipelineState;
	};


	class TD3D12PipelineStateCache : public TD3D12DeviceChild
	{
	public:
		TD3D12PipelineStateCache(ID3D12Device* InDevice) : TD3D12DeviceChild(InDevice) {}
		//TD3D12PipelineStateCache() = delete;
		TD3D12GraphicsPipelineState* FindInLoadedCache(const TGraphicsPipelineStateInitializer& initializer, const TD3D12RootSignature* rootSignature, TD3D12GraphicsPipelineStateDesc& outDesc);
		D3D12PipelineState* CreateAndAddToCache(const TD3D12GraphicsPipelineStateDesc& desc);
		
		
	private:
		template <typename TDesc, typename TValue = D3D12PipelineState*>
		using TPipelineCache = HashMap<TDesc, TValue>;
		TPipelineCache<TD3D12GraphicsPipelineStateDesc> GraphicsPipelineStateCache;
		//TPipelineCache<TD3D12ComputePipelineStateDesc> ComputePipelineStateCache;
	};
	
}

