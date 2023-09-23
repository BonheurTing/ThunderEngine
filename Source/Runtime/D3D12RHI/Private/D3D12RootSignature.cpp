#include "D3D12RootSignature.h"
#include "d3dx12.h"
#include "RHIHelper.h"

namespace Thunder
{
	TD3D12RootSignature::TD3D12RootSignature(ID3D12Device* InParent, const TShaderRegisterCounts& shaderRC) : TD3D12DeviceChild(InParent)
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(ParentDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
	}

	void TD3D12RootSignatureManager::Destroy()
	{
		for (auto Iter = RootSignatureMap.begin(); Iter != RootSignatureMap.end(); ++Iter)
		{
			TMemory::Destroy(Iter->second);
		}
		RootSignatureMap.clear();
	}

	TD3D12RootSignature* TD3D12RootSignatureManager::GetRootSignature(const TShaderRegisterCounts& shaderRC)
	{
		ScopeLock lock(MapMutex);
		if (RootSignatureMap.contains(shaderRC))
		{
			TD3D12RootSignature* pRootSignature = RootSignatureMap[shaderRC];
			return pRootSignature;
		}
		// Create a new root signature and return it.
		return CreateRootSignature(shaderRC);
	}

	TD3D12RootSignature* TD3D12RootSignatureManager::CreateRootSignature(const TShaderRegisterCounts& shaderRC)
	{
		const auto pRootSignature = new (TMemory::Malloc<TD3D12RootSignature>()) TD3D12RootSignature{ParentDevice, shaderRC};
		RootSignatureMap[shaderRC] = pRootSignature;
		return pRootSignature;
	}
}

