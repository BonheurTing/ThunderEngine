#pragma once
#include "RHI.h"
#include "D3D12RHICommon.h"
#include "CoreMinimal.h"

struct ID3D12RootSignature;

namespace Thunder
{
	constexpr uint32 kInvalidRootParameterIndex = ~0u;
	class TD3D12RootSignature : public TD3D12DeviceChild
	{
	public:
		TD3D12RootSignature() = delete;
		TD3D12RootSignature(ID3D12Device* InParent, const TShaderRegisterCounts& shaderRC);
		_NODISCARD_ ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }

		uint32 GetSRVRootParameterIndex() const { return SRVRootParameterIndex; }
		uint32 GetSamplerRootParameterIndex() const { return SamplerRootParameterIndex; }
		uint32 GetUAVRootParameterIndex() const { return UAVRootParameterIndex; }
		uint32 GetCBVRootParameterIndexBegin() const { return CBVRootParameterIndexBegin; }

		static constexpr uint32 MaxRootParameters = 32;	// Arbitrary max, increase as needed.

	private:
		ComPtr<ID3D12RootSignature> RootSignature;
		uint32 SRVRootParameterIndex = kInvalidRootParameterIndex;
		uint32 SamplerRootParameterIndex = kInvalidRootParameterIndex;
		uint32 UAVRootParameterIndex = kInvalidRootParameterIndex;
		uint32 CBVRootParameterIndexBegin = kInvalidRootParameterIndex;
	};
	
	class TD3D12RootSignatureManager : public TD3D12DeviceChild
	{
	public:
		explicit TD3D12RootSignatureManager(ID3D12Device* InParent)
			: TD3D12DeviceChild(InParent)
		{
		}
		~TD3D12RootSignatureManager()
		{
			if (!RootSignatureMap.empty())
			{
				Destroy();
			}
		}
		void Destroy();

		TD3D12RootSignature* GetRootSignature(const TShaderRegisterCounts& shaderRC);

	private:
		Mutex MapMutex;
		TD3D12RootSignature* CreateRootSignature(const TShaderRegisterCounts& shaderRC);

		THashMap<TShaderRegisterCounts, TD3D12RootSignature*> RootSignatureMap;

	};
}

