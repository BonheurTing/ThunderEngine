#pragma once
#include "Assertion.h"
#include "D3D12RHICommon.h"
#include "CoreMinimal.h"

struct ID3D12RootSignature;

namespace Thunder
{
	class TD3D12RootSignature : public TD3D12DeviceChild
	{
	public:
		TD3D12RootSignature(ID3D12Device* InParent, const TShaderRegisterCounts& shaderRC);
	private:
		ComPtr<ID3D12RootSignature> RootSignature;
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
			TAssert(RootSignatureMap.size() == 0);
		}
		void Destroy();

		TD3D12RootSignature* GetRootSignature(const TShaderRegisterCounts& shaderRC);

	private:
		Mutex MapMutex;
		TD3D12RootSignature* CreateRootSignature(const TShaderRegisterCounts& shaderRC);

		HashMap<TShaderRegisterCounts, TD3D12RootSignature*> RootSignatureMap;

	};
}

